# Use official Node.js LTS image with specific version
FROM node:18.20-alpine

# Create app directory
WORKDIR /app

# Add non-root user for security
RUN addgroup -g 1001 -S nodejs && \
    adduser -S minifier -u 1001 -G nodejs

# Copy package files
COPY package*.json ./

# Install dependencies
RUN npm ci --only=production && \
    npm cache clean --force

# Copy application code
COPY enhanced-server.js ./server.js

# Set proper ownership
RUN chown -R minifier:nodejs /app

# Switch to non-root user
USER minifier

# Health check (using wget since curl is not available in Alpine)
HEALTHCHECK --interval=30s --timeout=10s --start-period=5s --retries=3 \
  CMD wget --no-verbose --tries=1 --spider http://localhost:3002/health || exit 1

# Expose port
EXPOSE 3002

# Set production environment
ENV NODE_ENV=production
ENV PORT=3002

# Start the application
CMD ["node", "server.js"]
