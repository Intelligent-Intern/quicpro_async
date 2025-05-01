#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "${BASH_SOURCE[0]}")/../../"

source infra/scripts/utils.sh

help_box "Project Make Targets" \
  "build      → Builds the PECL extension (requires PHP ≥ 8.4 CLI)" \
  "unit       → Runs PHPUnit tests for PHP 8.1–8.4 using Docker containers" \
  "fuzz       → Starts C-layer fuzz harnesses to expose edge-case bugs" \
  "benchmark  → Executes performance benchmarks via Pulumi and Hetzner Cloud - requires API key - we run real servers" \
  "deploy     → Publishes artifacts or triggers GitHub Actions workflow" \
  "help       → Displays this help overview"
