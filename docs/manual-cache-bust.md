# Manual Docker Cache Busting

## How to Manually Bust Docker Cache

When you need to force a rebuild of the Docker layers (e.g., to update CMake or other dependencies), follow these steps:

1. **Go to GitHub Actions**
   - Navigate to your repository on GitHub
   - Click on the "Actions" tab

2. **Run Workflow Manually**
   - Select "CI/CD Pipeline" from the workflow list
   - Click "Run workflow" button
   - You'll see a form with "Cache version" field

3. **Change Cache Version**
   - Change the value from `1` to any other value (e.g., `2`, `3`, or any string)
   - Click "Run workflow"

4. **How it Works**
   - The `CACHEBUST` ARG in the Dockerfile will receive this new value
   - Docker will see this as a change and rebuild from that layer onwards
   - This updates CMake and all subsequent layers

## Example

- Default cache version: `1`
- To bust cache, change to: `2` (or any different value)
- Next time you need to bust cache again, change to: `3` (and so on)

The cache will only be busted when you manually trigger the workflow with a different cache version value. 