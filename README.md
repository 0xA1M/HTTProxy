# HTTP/HTTPS Proxy Server in C

This project aims to create a lightweight and scalable HTTP/HTTPS proxy server in C. The focus is on implementing core functionalities to ensure the proxy server works effectively.

## Features

### Core Features (TODO List):

- [ ] **HTTP/1.1 Support**: Handle HTTP/1.1 requests, methods, headers, and response codes.
- [ ] **Persistent Connections**: Support HTTP/1.1 Keep-Alive for multiple requests over a single connection.
- [x] **Multithreading**: Handle multiple client connections concurrently for better scalability.
- [ ] **Partial `recv/send`**: Support partial data transfers to efficiently handle large requests/responses.
- [ ] **Chunked Transfer Encoding**: Process HTTP chunked transfers for streaming and large content.
- [ ] **Binary Data Support**: Handle binary data transfers for non-text content.
- [ ] **Compression Support**: Implement support for major compression/decompression (gzip/deflate/br) standards in HTTP requests and responses.
- [ ] **HTTPS Support**: Implement SSL/TLS tunneling for HTTPS traffic using the `CONNECT` method.

### Quality of life features

- [ ] **Logging**: Track requests, responses, errors, and connection details for monitoring and debugging via [clog](https://github.com/0xA1M/clog).
- [ ] **Request Throttling**: Limit client request rates to prevent abuse and ensure resource fairness.
- [ ] **Timeout Management**: Handle connection/request timeouts to prevent resource waste.
- [ ] **Text-Based User Interface (TUI)**: Real-time console for monitoring server activity and logs.
- [ ] **Caching**: Store responses for faster retrieval of frequently accessed content.
- [ ] **Web filtering**: Implement a feature to block access to specified websites.

## Installation

1. **Clone the repository**:
   ```bash
   git clone https://github.com/0xA1M/HTTProxy/
   ```
2. **Build the project**:
   ```bash
   cd HTTProxy
   make
   ```

## Usage

To run the proxy server, use the following command, specifying the port number on which the server will listen for incoming connections:
```bash
./httproxy <port_number>
```

Configure your browser to use the proxy server by setting the HTTP proxy settings to point to the server's address and port.

## System Requirements

- Linux (no Windows or macOS support)

## License

This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.
