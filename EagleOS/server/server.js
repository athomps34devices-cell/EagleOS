"use strict";

const http = require("http");
const fs = require("fs");
const path = require("path");

const SERVER_CONFIG = {
    host: "127.0.0.1",
    port: 8000,
    root: path.resolve(__dirname, ".."),
    defaultPage: "/installer/installer.html"
};

const MIME_TYPES = {
    ".html": "text/html; charset=utf-8",
    ".css": "text/css; charset=utf-8",
    ".js": "application/javascript; charset=utf-8",
    ".json": "application/json; charset=utf-8",
    ".png": "image/png",
    ".jpg": "image/jpeg",
    ".jpeg": "image/jpeg",
    ".gif": "image/gif",
    ".svg": "image/svg+xml",
    ".webp": "image/webp",
    ".ico": "image/x-icon",
    ".woff": "font/woff",
    ".woff2": "font/woff2",
    ".ttf": "font/ttf",
    ".otf": "font/otf",
    ".mp3": "audio/mpeg",
    ".wav": "audio/wav",
    ".mp4": "video/mp4",
    ".webm": "video/webm",
    ".txt": "text/plain; charset=utf-8",
    ".xml": "application/xml; charset=utf-8",
    ".wasm": "application/wasm"
};

function sendResponse(res, statusCode, content, contentType = "text/plain; charset=utf-8") {
    res.writeHead(statusCode, {
        "Content-Type": contentType,
        "Cache-Control": "no-cache",
        "X-Content-Type-Options": "nosniff",
        "X-Frame-Options": "SAMEORIGIN",
        "Referrer-Policy": "strict-origin-when-cross-origin"
    });

    res.end(content);
}

function resolveRequestPath(requestUrl) {
    let pathname;

    try {
        pathname = decodeURIComponent(
            new URL(requestUrl, `http://${SERVER_CONFIG.host}`).pathname
        );
    } catch {
        return null;
    }

    if (pathname === "/") {
        pathname = SERVER_CONFIG.defaultPage;
    }

    const requestedPath = path.resolve(
        SERVER_CONFIG.root,
        `.${pathname}`
    );

    const relativePath = path.relative(
        SERVER_CONFIG.root,
        requestedPath
    );

    if (
        relativePath === ".." ||
        relativePath.startsWith(`..${path.sep}`) ||
        path.isAbsolute(relativePath)
    ) {
        return null;
    }

    return requestedPath;
}

function serveFile(req, res) {
    const filePath = resolveRequestPath(req.url);

    if (!filePath) {
        sendResponse(res, 403, "Forbidden");
        return;
    }

    fs.stat(filePath, (statError, stats) => {
        if (statError || !stats.isFile()) {
            sendResponse(res, 404, "404 - File Not Found");
            return;
        }

        const extension = path.extname(filePath).toLowerCase();

        const contentType = MIME_TYPES[extension] ||
            "application/octet-stream";

        res.writeHead(200, {
            "Content-Type": contentType,
            "Content-Length": stats.size,
            "Cache-Control": "no-cache",
            "X-Content-Type-Options": "nosniff",
            "X-Frame-Options": "SAMEORIGIN",
            "Referrer-Policy": "strict-origin-when-cross-origin"
        });

        if (req.method === "HEAD") {
            res.end();
            return;
        }

        const stream = fs.createReadStream(filePath);

        stream.on("error", error => {
            console.error("[SERVER] File read error:", error.message);

            if (!res.headersSent) {
                sendResponse(res, 500, "500 - Internal Server Error");
            } else {
                res.destroy();
            }
        });

        stream.pipe(res);
    });
}

function handleRequest(req, res) {
    console.log(`[${new Date().toLocaleTimeString()}] ${req.method} ${req.url}`);

    if (req.method !== "GET" && req.method !== "HEAD") {
        sendResponse(res, 405, "405 - Method Not Allowed");
        return;
    }

    serveFile(req, res);
}

const server = http.createServer(handleRequest);

server.on("error", error => {
    if (error.code === "EADDRINUSE") {
        console.error(`[SERVER] Port ${SERVER_CONFIG.port} is already in use.`);
        console.error("[SERVER] Close the other server or choose another port.");
    } else {
        console.error("[SERVER] Server error:", error);
    }
});

server.listen(SERVER_CONFIG.port, SERVER_CONFIG.host, () => {
    console.log("");
    console.log("==========================================");
    console.log("           EagleOS Development");
    console.log("==========================================");
    console.log("");
    console.log("Status: ONLINE");
    console.log(`Address: http://localhost:${SERVER_CONFIG.port}`);
    console.log(`Installer: http://localhost:${SERVER_CONFIG.port}/installer/installer.html`);
    console.log(`Project root: ${SERVER_CONFIG.root}`);
    console.log("");
    console.log("Press CTRL+C to stop the server.");
    console.log("==========================================");
    console.log("");
});