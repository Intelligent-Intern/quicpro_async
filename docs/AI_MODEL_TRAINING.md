# Guide: Distributed AI Training in PHP with `quicpro_async`

This document describes the process of training a complex AI model (based on I-JEPA) across multiple GPUs using a purely PHP-based stack, powered by the `quicpro_async` extension. The approach divides the task into two main components: a **master script** that orchestrates the entire process, and a **worker script** that executes the actual training logic on each individual GPU.

---

### Step 1: The Master Script (`train_master.php`) – The Conductor

The master script acts as the central supervisor for the entire training cluster. Its sole responsibility is to configure, start, and monitor the worker processes. It does not perform any AI computations itself.

#### 1.1 Defining Hyperparameters

First, we define all the necessary hyperparameters for our training. These are the fundamental settings that determine the model's behavior and outcome. Note that we are using the `quicpro-fs://` stream wrapper to point to our dataset, ensuring it is loaded through the high-performance native object store.

```php
$hyperparameters = [
    'epochs' => 100,
    'batch_size' => 256,
    'learning_rate' => 1e-4,
    'model_arch' => 'vit_huge_patch14',
    // Use the native object store for high-performance data loading
    'dataset_path' => 'quicpro-fs://datasets/imagenet',
    'num_gpus' => 8, // We want to train on 8 GPUs
];
```

#### 1.2 Configuring `Quicpro\Cluster`

Next, we configure the `Quicpro\Cluster` supervisor. This is a class provided by the C extension that handles the management of multi-process applications.

```php
$clusterConfig = Config::new([
    // We want one worker process per GPU.
    'cluster_workers' => $hyperparameters['num_gpus'],

    // Each worker should be pinned to a specific GPU.
    'worker_gpu_affinity_map' => '0:0, 1:1, 2:2, 3:3, 4:4, 5:5, 6:6, 7:7',

    // We want crashed workers to be restarted automatically.
    'cluster_restart_crashed_workers' => true,
]);
```

#### 1.3 Starting and Monitoring the Cluster

Finally, we instantiate and start the cluster.

```php
// The script that each worker should execute.
$workerScript = __DIR__ . '/train_worker.php';

// The hyperparameters are passed as a JSON string to each worker.
$cluster = new Cluster($workerScript, json_encode($hyperparameters), $clusterConfig);

// The run() command blocks, and the master process becomes the supervisor.
$cluster->run();
```

---

### Step 2: The Worker Script (`train_worker.php`) – The Engine

This script contains the core logic for the AI training. It is executed by each worker process spawned by the `Quicpro\Cluster`. Each worker operates independently but in a coordinated fashion on its assigned GPU.

#### 2.1 Initializing the Distributed Process

Each worker must first understand its role within the collective. The `quicpro_async` extension automatically provides this context via environment variables.

```php
// Hypothetical high-level classes provided by quicpro_async.
use Quicpro\AI\Collective;

// Each worker gets its configuration and its rank from the master process.
$rank = (int)getenv('QUICPRO_WORKER_ID');
$worldSize = (int)getenv('QUICPRO_CLUSTER_SIZE');
$hyperparameters = json_decode(getenv('QUICPRO_PAYLOAD'), true);

// This initializes the inter-process communication backend (e.g., NCCL/RCCL).
// The C extension handles the handshake and discovery of the master address/port.
Collective::initProcessGroup('nccl', $rank, $worldSize);
```

#### 2.2 Defining the Model, Loss, and Optimizer

With the distributed environment set up, each worker instantiates the AI model, the loss function, and the optimizer. The PHP API is designed to mirror familiar frameworks like PyTorch.

```php
use Quicpro\AI\Model\VisionTransformer;
use Quicpro\AI\Loss\IJEPALoss;
use Quicpro\AI\Optimizer\AdamW;
use Quicpro\AI\DistributedDataParallel as DDP;

echo "[Worker {$rank}] Initializing model on GPU {$rank}...\n";

// The C-level implementation creates the model on the assigned GPU.
$model = new VisionTransformer($hyperparameters['model_arch']);
$lossFn = new IJEPALoss();
$optimizer = new AdamW($model->parameters(), 'lr' => $hyperparameters['learning_rate']);

// The model is wrapped with DistributedDataParallel (DDP). The C extension
// handles the automatic synchronization of gradients across all GPUs
// during the backward pass.
$model = new DDP($model, 'device_ids' => [$rank]);
```

#### 2.3 Loading the Data

Each worker initializes its own data loader. Crucially, it uses the `quicpro-fs://` URI, and the `ImageNetLoader` is configured to only load its unique shard of the total dataset, ensuring no data is processed twice in one epoch.

```php
use Quicpro\AI\Dataset\ImageNetLoader;

echo "[Worker {$rank}] Loading data from object store...\n";
$dataset = new ImageNetLoader(
    $hyperparameters['dataset_path'],
    'batch_size' => $hyperparameters['batch_size'],
    'num_workers' => $worldSize, // Total number of data shards
    'rank' => $rank             // Which shard this worker should load
);
```

#### 2.4 The Training Loop

This is the core of the worker's job. It iterates through the dataset for a specified number of epochs, performing the forward pass, calculating the loss, and executing the backward pass to update the model's weights.

```php
for ($epoch = 0; $epoch < $hyperparameters['epochs']; $epoch++) {
    foreach ($dataset as $i => $batch) {
        // Data is automatically moved to the correct GPU by the C extension.
        // $batch['images'] and $batch['masks'] are internal GPU tensor handles.

        // Forward pass
        $predicted_embeddings = $model->forward($batch['images'], $batch['masks']);

        // Calculate loss
        $loss = $lossFn->calculate($predicted_embeddings, $batch['images']);

        // Reset optimizer gradients
        $optimizer->zeroGrad();

        // Backward pass
        // DDP automatically syncs gradients across all GPUs via an `all_reduce`
        // operation in the C core.
        $loss->backward();

        // Update model weights
        $optimizer->step();

        // Logging (only on the main worker to avoid clutter)
        if ($rank === 0 && $i % 50 === 0) {
            // The .item() method copies the loss value from GPU to CPU memory
            // to make it accessible in PHP.
            printf(
                "Epoch: %d | Batch: %d | Loss: %.4f\n",
                $epoch,
                $i,
                $loss->item()
            );
        }
    }
}

echo "[Worker {$rank}] Training complete.\n";
```