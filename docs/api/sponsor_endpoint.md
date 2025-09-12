# Sponsor API Documentation

## Overview

The Sponsor API provides functionality for submitting sponsor applications with automatic MongoDB storage and bank information response.

## Endpoint

### POST /api/v2/sponsor-submit

Submit a sponsor application with required and optional fields.

#### Request

**Content-Type:** `application/json`

**Body:**

```json
{
  "name": "string (required)",
  "email": "string (required)",
  "mobile": "string (required)",
  "tier": "string (required)",
  "amount": "number (required)",
  "company": "string (optional)"
}
```

#### Field Descriptions

| Field     | Type   | Required | Description                                                        |
| --------- | ------ | -------- | ------------------------------------------------------------------ |
| `name`    | string | ✅       | Full name of the sponsor                                           |
| `email`   | string | ✅       | Email address for contact                                          |
| `mobile`  | string | ✅       | Mobile phone number                                                |
| `tier`    | string | ✅       | Sponsorship tier/plan (e.g., "basic", "premium", "gold", "silver") |
| `amount`  | number | ✅       | Amount in IRR (Iranian Rial)                                       |
| `company` | string | ❌       | Company name (optional)                                            |

#### Example Request

```bash
curl --location 'http://localhost:3000/api/v2/sponsor-submit' \
--header 'Content-Type: application/json' \
--data-raw '{
    "name": "Ahmad Mohammadi",
    "email": "ahmad@example.com",
    "mobile": "09123456789",
    "tier": "premium",
    "amount": 2500000,
    "company": "Tech Corp"
}'
```

#### Response

**Success Response (200 OK):**

```json
{
  "success": true,
  "message": "فرم حمایت با موفقیت ارسال و ذخیره شد",
  "submissionId": "68b05d4abb79f500190b8a92",
  "savedToDatabase": true,
  "bankInfo": {
    "bankName": "بانک پاسارگاد",
    "accountNumber": "3047-9711-6543-2",
    "iban": "IR64 0570 3047 9711 6543 2",
    "accountHolder": "هاتف پروژه",
    "swift": "PASAIRTHXXX",
    "currency": "IRR"
  },
  "note": "لطفاً پس از واریز مبلغ، رسید پرداخت را به آدرس ایمیل sponsors@hatef.ir ارسال کنید."
}
```

#### Response Fields

| Field             | Type    | Description                                    |
| ----------------- | ------- | ---------------------------------------------- |
| `success`         | boolean | Always `true` for successful submissions       |
| `message`         | string  | Success message in Persian                     |
| `submissionId`    | string  | MongoDB ObjectId of the stored record          |
| `savedToDatabase` | boolean | Whether data was successfully saved to MongoDB |
| `bankInfo`        | object  | Bank account details for payment               |
| `note`            | string  | Payment instructions                           |

#### Bank Information

The API returns complete bank details for payment:

- **Bank:** Pasargad Bank (بانک پاسارگاد)
- **Account Number:** 3047-9711-6543-2
- **IBAN:** IR64 0570 3047 9711 6543 2
- **Account Holder:** هاتف پروژه
- **SWIFT:** PASAIRTHXXX
- **Currency:** IRR (Iranian Rial)

## Data Storage

### MongoDB Schema

Sponsor data is stored in the `search-engine.sponsors` collection with the following structure:

```json
{
  "_id": "ObjectId",
  "fullName": "string",
  "email": "string",
  "mobile": "string",
  "plan": "string",
  "amount": "number",
  "company": "string (optional)",
  "ipAddress": "string",
  "userAgent": "string",
  "submissionTime": "ISODate",
  "lastModified": "ISODate",
  "status": "string (PENDING/VERIFIED/REJECTED/CANCELLED)",
  "currency": "string (IRR)"
}
```

### Backend Tracking

The API automatically captures additional data:

- **IP Address:** Client IP address
- **User Agent:** Browser/client information
- **Submission Time:** Timestamp when form was submitted
- **Last Modified:** Timestamp of last update
- **Status:** Current status (defaults to "PENDING")

## Error Handling

### Validation Errors

If required fields are missing or invalid:

```json
{
  "success": false,
  "message": "خطا در اعتبارسنجی داده‌ها",
  "errors": {
    "name": "نام الزامی است",
    "email": "ایمیل معتبر الزامی است"
  }
}
```

### Database Errors

If MongoDB connection fails:

```json
{
  "success": true,
  "message": "فرم حمایت دریافت شد",
  "submissionId": "temp_1756388153",
  "savedToDatabase": false,
  "bankInfo": { ... },
  "note": "لطفاً پس از واریز مبلغ، رسید پرداخت را به آدرس ایمیل sponsors@hatef.ir ارسال کنید."
}
```

## Implementation Details

### MongoDB Integration

The API uses the MongoDB C++ driver with proper singleton pattern:

```cpp
// Proper MongoDB initialization
mongocxx::instance& instance = MongoDBInstance::getInstance();
mongocxx::uri uri{connectionString};
client_ = std::make_unique<mongocxx::client>(uri);
```

### Key Files

- **Controller:** `src/controllers/HomeController.cpp`
- **Storage:** `src/storage/SponsorStorage.cpp`
- **Model:** `include/search_engine/storage/SponsorProfile.h`
- **Frontend:** `public/sponsor.js`, `templates/sponsor.inja`

## Testing

### Test with curl

```bash
# Basic submission
curl --location 'http://localhost:3000/api/v2/sponsor-submit' \
--header 'Content-Type: application/json' \
--data-raw '{
    "name": "Test User",
    "email": "test@example.com",
    "mobile": "09123456789",
    "tier": "basic",
    "amount": 1000000
}'

# With company
curl --location 'http://localhost:3000/api/v2/sponsor-submit' \
--header 'Content-Type: application/json' \
--data-raw '{
    "name": "Ali Rezaei",
    "email": "ali@example.com",
    "mobile": "09351234567",
    "tier": "gold",
    "amount": 3000000,
    "company": "Digital Solutions"
}'
```

### Verify Database

```bash
# Check stored records
docker exec mongodb_test mongosh --username admin --password password123 \
--eval "use('search-engine'); db.sponsors.find().sort({_id: -1}).pretty()"
```

## Security Considerations

- Input validation for all fields
- SQL injection protection via MongoDB driver
- Rate limiting should be implemented
- HTTPS recommended for production
- Email validation for contact information

## Future Enhancements

- Email confirmation system
- Payment gateway integration
- Admin dashboard for sponsor management
- Automated status updates
- Analytics and reporting
