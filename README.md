# CS 1541 Project Trace File Generator

This package generates instruction traces from 32-bit x86 binaries using the [Pin binary instrumentation tool](https://www.intel.com/content/www/us/en/developer/articles/tool/pin-a-dynamic-binary-instrumentation-tool.html).

## Prerequisites

* Genuine Intel CPU (AMD CPUs, even if they support x86, are known to cause issues with Pin).
* Pin binary instrumentation tool: [Download Link](https://www.intel.com/content/www/us/en/developer/articles/tool/pin-a-binary-instrumentation-tool-downloads.html)

## How-to-use

1. Edit [makefile](makefile) to set PIN_ROOT variable to the root of your Pin installation:
   ```
   PIN_ROOT = [Your Pin installation root]
   ```
2. (Optional) Edit [makefile.rules](makefile.rules) to set TRACED_APP_ROOTS variable to your list of traced apps:
   ```
   TRACED_APP_ROOTS := linked-list array [any additional apps with corresponding .cpp source code]
   ```
3. (Optional) Try building the instruction tracing Pin tool:
   ```
   make build
   ```
4. Generate traces from TRACED_APP_ROOTS:
   ```
   make runs
   ```
   The "runs" target automatically builds the Pin tool and the app binaries before running the app binaries with the Pin tool instrumentation.  All the traces are generated under the traces/ folder.
