<?php

/*
1.  FetchUrlContent: Fetches content from a URL.
    - Input: { "url": "string" }
    - Output: { "http_status": int, "content_type": "string", "body": "bytes|string" }
2.  ExtractTextFromHtml: Extracts plain text from HTML content.
    - Input: { "html_content": "string" }
    - Output: { "extracted_text": "string" }
3.  SummarizeText: Creates a summary of a text.
    - Input: { "text_to_summarize": "string", "max_length_words": "int_optional", "style": "string_optional_bullet_points|paragraph" }
    - Output: { "summary": "string" }
4.  TranslateText: Translates text to another language.
    - Input: { "text": "string", "target_language_code": "string", "source_language_code": "string_optional" }
    - Output: { "translated_text": "string", "detected_source_language": "string" }
5.  DetectLanguage: Detects the language of a text.
    - Input: { "text_sample": "string" }
    - Output: { "language_code": "string", "confidence": "float" }
6.  ExtractKeywords: Extracts keywords from a text.
    - Input: { "text_content": "string", "max_keywords": "int_optional" }
    - Output: { "keywords": ["string"] }
7.  SentimentAnalysis: Analyzes the sentiment of a text.
    - Input: { "text_to_analyze": "string" }
    - Output: { "sentiment_label": "string_positive|negative|neutral", "sentiment_score": "float" }
8.  NamedEntityRecognition (NER): Recognizes named entities (people, places, organizations).
    - Input: { "text": "string" }
    - Output: { "entities": [{"text": "string", "type": "string", "offset": "int"}] }
9.  TextClassification: Classifies text into predefined categories.
    - Input: { "text": "string", "categories": ["string_optional_predefined_model"] }
    - Output: { "classification": "string", "scores": {"category": "float"} }
10. QuestionAnswering: Answers a question based on a context text.
    - Input: { "context_text": "string", "question": "string" }
    - Output: { "answer": "string", "confidence": "float" }
11. GenerateText (LLM Call): Generates text based on a prompt.
    - Input: { "prompt": "string", "model_identifier": "string_optional", "max_tokens": "int_optional", "temperature": "float_optional" }
    - Output: { "generated_text": "string" }
12. ImageClassification: Classifies the content of an image.
    - Input: { "image_bytes": "bytes" | "image_url": "string" }
    - Output: { "labels": [{"name": "string", "score": "float"}] }
13. ObjectDetectionOnImage: Detects objects in an image.
    - Input: { "image_bytes": "bytes" | "image_url": "string" }
    - Output: { "objects": [{"label": "string", "score": "float", "bounding_box": [x,y,w,h]}] }
14. OcrImage: Extracts text from an image (Optical Character Recognition).
    - Input: { "image_bytes": "bytes" | "image_url": "string", "language_hint": "string_optional" }
    - Output: { "extracted_text": "string" }
15. DataValidation: Validates data against a given schema.
    - Input: { "data_to_validate": "any_php_array_or_object", "schema_identifier": "string_schema_name" }
    - Output: { "is_valid": "bool", "errors": ["string_optional"] }
16. DataTransformation (Simple): Performs simple transformations.
    - Input: { "data_input": "array", "transformation_rules": "array_or_string_dsl" }
    - Output: { "data_output": "array" }
17. CalculateEmbedding: Generates a vector embedding for text or data.
    - Input: { "content_to_embed": "string|bytes", "embedding_model_hint": "string_optional" }
    - Output: { "embedding_vector": ["float"] }
18. StoreDataRecord: Stores a data record in a predefined target.
    - Input: { "record_data": "array|object", "storage_target_id": "string", "collection_name": "string_optional" }
    - Output: { "record_id": "string", "status": "string_success|failure" }
19. FetchDataRecord: Fetches a data record from a predefined target.
    - Input: { "record_id_to_fetch": "string", "storage_target_id": "string", "collection_name": "string_optional" }
    - Output: { "record_data": "array|object", "status": "string_found|not_found" }
20. ConditionalLogic: (Meta-step) Executes the next step only if a condition is met.
    - Input: { "condition_field_path": "string_from_previous_output", "operator": "string_equals|contains|gt|lt", "value_to_compare": "any" }
    - Output: { "condition_met": "bool" }
*/

$initialData = ['input_url' => 'https://en.wikipedia.org/wiki/QUIC'];

$pipelineDefinition = [
    ['tool' => 'FetchUrlContent',         'input_map' => ['url' => '@initial.input_url']],
    ['tool' => 'ExtractTextFromHtml',     'input_map' => ['html_content' => '@previous.body']],
    ['tool' => 'SummarizeText',           'params'    => ['max_length_words' => 50]], // Input is @previous.extracted_text (convention)
    ['tool' => 'ExtractKeywords',         'params'    => ['max_keywords' => 5]],      // Input is @previous.extracted_text
];

$result = Quicpro\PipelineOrchestrator::run($initialData, $pipelineDefinition);

if ($result->isSuccess()) {
    echo "Summary: " . ($result->getOutputOfStep('SummarizeText')['summary'] ?? 'N/A') . "\n";
    echo "Keywords: " . implode(', ', $result->getOutputOfStep('ExtractKeywords')['keywords'] ?? ['N/A']) . "\n";
} else {
    echo "Pipeline Error: " . $result->getErrorMessage() . "\n";
}
