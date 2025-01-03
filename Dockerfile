# Use the official Go image for ARM architecture
FROM arm32v7/golang:1.23 AS builder

# Set the Current Working Directory inside the container
WORKDIR /app

# Copy the Go source code into the container (from the server directory)
COPY server/ .

# Download Go dependencies (optional, depending on your project setup)
RUN go mod tidy

# Build the Go app
RUN go build -o main .

# Create a smaller image to run the built Go application
FROM arm32v7/alpine:3.15

# Set the working directory in the second stage
WORKDIR /root/

# Copy the Go app from the builder stage
COPY --from=builder /app/main .

# Expose the port the app runs on (adjust as necessary)
EXPOSE 8080

# Command to run the Go app
CMD ["./main"]
