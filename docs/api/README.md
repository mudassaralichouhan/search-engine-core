# Search API Documentation

This directory contains the REST API documentation for the search engine.

## Files

- **search_endpoint.md** - REST API specification for the `/search` endpoint
- **search_response.schema.json** - JSON Schema definition for the search response format
- **examples/** - Directory containing example JSON responses for testing

## Validation

To validate example JSON files against the schema:

```bash
npm run validate-schema
```

To check formatting:

```bash
npm run format:check
```

To automatically fix formatting:

```bash
npm run format
```

## CI/CD

The GitHub Actions workflow at `.github/workflows/api-validation.yml` automatically:
- Validates all JSON examples against the schema
- Checks code formatting with Prettier
- Verifies required API documentation files exist

This ensures the API documentation remains consistent and valid. 