#!/usr/bin/env node

const fs = require('fs');
const path = require('path');
const Ajv = require('ajv');
const addFormats = require('ajv-formats');

// Initialize AJV with format support
const ajv = new Ajv({ strict: true });
addFormats(ajv);

// Schema mapping for different example files
const schemaMapping = {
  'search_response_example.json': 'search_response.schema.json',
  'crawler_response_example.json': 'crawler_response.schema.json'
};

// Find all example JSON files
const examplesDir = path.join(__dirname, '..', 'docs', 'api', 'examples');
const schemasDir = path.join(__dirname, '..', 'docs', 'api');
let hasErrors = false;

// Create examples directory if it doesn't exist
if (!fs.existsSync(examplesDir)) {
  fs.mkdirSync(examplesDir, { recursive: true });
}

// Get all JSON files in the examples directory
const exampleFiles = fs.readdirSync(examplesDir).filter(file => file.endsWith('.json'));

if (exampleFiles.length === 0) {
  console.log('No example JSON files found in docs/api/examples/');
  console.log('Creating a default example...');
  
  // Create a default example
  const defaultExample = {
    meta: {
      total: 123,
      page: 1,
      pageSize: 10
    },
    results: [
      {
        url: "https://example.com/page",
        title: "Example Page Title",
        snippet: "This is a snippet of the content with <em>highlighted</em> search terms...",
        score: 0.987,
        timestamp: "2024-01-15T10:30:00Z"
      }
    ]
  };
  
  fs.writeFileSync(
    path.join(examplesDir, 'search_response_example.json'),
    JSON.stringify(defaultExample, null, 2)
  );
  
  exampleFiles.push('search_response_example.json');
}

// Validate each example file
exampleFiles.forEach(file => {
  const filePath = path.join(examplesDir, file);
  console.log(`\nValidating ${file}...`);
  
  // Get the corresponding schema for this file
  const schemaName = schemaMapping[file];
  if (!schemaName) {
    console.log(`⚠️  No schema mapping found for ${file}, skipping validation`);
    return;
  }
  
  const schemaPath = path.join(schemasDir, schemaName);
  
  // Check if schema exists
  if (!fs.existsSync(schemaPath)) {
    console.log(`⚠️  Schema ${schemaName} not found for ${file}, skipping validation`);
    return;
  }
  
  try {
    // Load the schema
    const schema = JSON.parse(fs.readFileSync(schemaPath, 'utf8'));
    
    // Compile the schema
    const validate = ajv.compile(schema);
    
    // Load and validate the example data
    const data = JSON.parse(fs.readFileSync(filePath, 'utf8'));
    const valid = validate(data);
    
    if (valid) {
      console.log(`✅ ${file} is valid`);
    } else {
      console.error(`❌ ${file} is invalid:`);
      console.error(JSON.stringify(validate.errors, null, 2));
      hasErrors = true;
    }
  } catch (error) {
    console.error(`❌ Error reading/parsing ${file}: ${error.message}`);
    hasErrors = true;
  }
});

// Exit with error code if validation failed
if (hasErrors) {
  process.exit(1);
} else {
  console.log('\n✅ All validations passed!');
  process.exit(0);
} 