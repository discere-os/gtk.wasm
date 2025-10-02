#!/usr/bin/env -S deno run --allow-read --allow-write --allow-net

/**
 * Static HTTP Server for GTK WebGPU Demo
 * Serves WASM files with proper MIME types and headers
 */

interface ServerConfig {
  port: number;
  hostname: string;
  enableCORS: boolean;
  enableCaching: boolean;
  verboseLogging: boolean;
}

const WASM_MIME_TYPES: Record<string, string> = {
  '.wasm': 'application/wasm',
  '.js': 'application/javascript',
  '.mjs': 'application/javascript',
  '.ts': 'application/typescript',
  '.html': 'text/html',
  '.css': 'text/css',
  '.json': 'application/json',
  '.png': 'image/png',
  '.jpg': 'image/jpeg',
  '.jpeg': 'image/jpeg',
  '.svg': 'image/svg+xml',
  '.ico': 'image/x-icon',
  '.md': 'text/markdown',
  '.txt': 'text/plain',
  '.map': 'application/json',
};

const SECURITY_HEADERS = {
  'Cross-Origin-Embedder-Policy': 'require-corp',
  'Cross-Origin-Opener-Policy': 'same-origin',
  'Cross-Origin-Resource-Policy': 'cross-origin',
  'X-Content-Type-Options': 'nosniff',
  'X-Frame-Options': 'SAMEORIGIN',
  'Referrer-Policy': 'strict-origin-when-cross-origin',
};

const CORS_HEADERS = {
  'Access-Control-Allow-Origin': '*',
  'Access-Control-Allow-Methods': 'GET, POST, OPTIONS',
  'Access-Control-Allow-Headers': 'Content-Type, Authorization',
  'Access-Control-Max-Age': '86400',
};

class StaticServer {
  private config: ServerConfig;
  private rootDir: string;

  constructor(config: ServerConfig, rootDir: string = Deno.cwd()) {
    this.config = config;
    this.rootDir = rootDir;
  }

  private getMimeType(filePath: string): string {
    const ext = '.' + filePath.split('.').pop()?.toLowerCase();
    return WASM_MIME_TYPES[ext] || 'application/octet-stream';
  }

  private getSecurityHeaders(filePath: string): Record<string, string> {
    const headers = { ...SECURITY_HEADERS };

    if (this.config.enableCORS) {
      Object.assign(headers, CORS_HEADERS);
    }

    // Special headers for WASM files
    if (filePath.endsWith('.wasm')) {
      headers['Content-Type'] = 'application/wasm';
      headers['Cache-Control'] = this.config.enableCaching ?
        'public, max-age=3600, immutable' : 'no-cache';
    }

    // Special headers for JS modules
    if (filePath.endsWith('.js') || filePath.endsWith('.mjs')) {
      headers['Content-Type'] = 'application/javascript';
      if (this.config.enableCaching) {
        headers['Cache-Control'] = 'public, max-age=1800';
      }
    }

    return headers;
  }

  private async serveFile(request: Request): Promise<Response> {
    const url = new URL(request.url);
    let filePath = url.pathname;

    // Remove leading slash and resolve path
    if (filePath.startsWith('/')) {
      filePath = filePath.slice(1);
    }

    // Default to index.html for directory requests
    if (filePath === '' || filePath.endsWith('/')) {
      filePath += 'demos/webgpu-widget-factory/index.html';
    }

    const fullPath = `${this.rootDir}/${filePath}`;

    try {
      // Check if file exists and is within root directory
      const realPath = await Deno.realPath(fullPath);
      const realRoot = await Deno.realPath(this.rootDir);

      if (!realPath.startsWith(realRoot)) {
        return new Response('Forbidden', { status: 403 });
      }

      const stat = await Deno.stat(realPath);

      if (stat.isDirectory) {
        // Try to serve index.html from directory
        const indexPath = `${realPath}/index.html`;
        try {
          await Deno.stat(indexPath);
          return this.serveFile(new Request(`${request.url}/index.html`));
        } catch {
          return this.serveDirectoryListing(filePath, realPath);
        }
      }

      const fileContent = await Deno.readFile(realPath);
      const mimeType = this.getMimeType(filePath);
      const headers = this.getSecurityHeaders(filePath);
      headers['Content-Type'] = mimeType;
      headers['Content-Length'] = fileContent.length.toString();

      if (this.config.verboseLogging) {
        console.log(`📄 Served: ${filePath} (${mimeType}, ${fileContent.length} bytes)`);
      }

      return new Response(fileContent, { headers });

    } catch (error) {
      if (error instanceof Deno.errors.NotFound) {
        if (this.config.verboseLogging) {
          console.log(`❌ Not found: ${filePath}`);
        }
        return new Response('Not Found', { status: 404 });
      }

      console.error(`💥 Server error for ${filePath}:`, error);
      return new Response('Internal Server Error', { status: 500 });
    }
  }

  private async serveDirectoryListing(requestPath: string, realPath: string): Promise<Response> {
    try {
      const entries = [];
      for await (const entry of Deno.readDir(realPath)) {
        entries.push({
          name: entry.name,
          isDirectory: entry.isDirectory,
          path: `${requestPath}/${entry.name}`.replace(/\/+/g, '/')
        });
      }

      entries.sort((a, b) => {
        if (a.isDirectory && !b.isDirectory) return -1;
        if (!a.isDirectory && b.isDirectory) return 1;
        return a.name.localeCompare(b.name);
      });

      const html = `
<!DOCTYPE html>
<html>
<head>
    <title>Directory: ${requestPath}</title>
    <meta charset="utf-8">
    <style>
        body { font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif; margin: 2rem; }
        .header { border-bottom: 1px solid #ccc; padding-bottom: 1rem; margin-bottom: 1rem; }
        .entry { display: flex; align-items: center; padding: 0.5rem 0; }
        .entry:hover { background-color: #f5f5f5; }
        .icon { margin-right: 0.5rem; }
        .name { flex: 1; }
        .size { color: #666; min-width: 80px; text-align: right; }
        a { text-decoration: none; color: #0066cc; }
        a:hover { text-decoration: underline; }
        .directory { font-weight: bold; }
    </style>
</head>
<body>
    <div class="header">
        <h1>📁 Directory: ${requestPath || '/'}</h1>
        <p>GTK WebGPU Demo Server</p>
    </div>

    ${requestPath !== '' ? `
    <div class="entry">
        <span class="icon">📁</span>
        <a href="../" class="directory">../</a>
    </div>
    ` : ''}

    ${entries.map(entry => `
    <div class="entry">
        <span class="icon">${entry.isDirectory ? '📁' : '📄'}</span>
        <a href="${entry.path}" class="${entry.isDirectory ? 'directory' : ''}">${entry.name}</a>
    </div>
    `).join('')}

    <div style="margin-top: 2rem; padding-top: 1rem; border-top: 1px solid #ccc; color: #666;">
        <small>GTK WebGPU Demo Server - ${new Date().toISOString()}</small>
    </div>
</body>
</html>`;

      return new Response(html, {
        headers: {
          'Content-Type': 'text/html; charset=utf-8',
          ...this.getSecurityHeaders(requestPath)
        }
      });
    } catch (error) {
      console.error('Error generating directory listing:', error);
      return new Response('Internal Server Error', { status: 500 });
    }
  }

  async start(): Promise<void> {
    const handler = async (request: Request): Promise<Response> => {
      const url = new URL(request.url);

      if (this.config.verboseLogging) {
        console.log(`${request.method} ${url.pathname}`);
      }

      // Handle CORS preflight
      if (request.method === 'OPTIONS') {
        return new Response(null, {
          status: 204,
          headers: this.config.enableCORS ? CORS_HEADERS : {}
        });
      }

      return this.serveFile(request);
    };

    const server = Deno.serve({
      port: this.config.port,
      hostname: this.config.hostname,
    }, handler);

    console.log(`🌐 GTK WebGPU Demo Server started`);
    console.log(`   URL: http://${this.config.hostname}:${this.config.port}/`);
    console.log(`   Root: ${this.rootDir}`);
    console.log(`   CORS: ${this.config.enableCORS ? 'Enabled' : 'Disabled'}`);
    console.log(`   Caching: ${this.config.enableCaching ? 'Enabled' : 'Disabled'}`);
    console.log(`   WebGPU Demo: http://${this.config.hostname}:${this.config.port}/demos/webgpu-widget-factory/`);
    console.log(``);
    console.log(`🔧 Headers configured for WebGPU + WASM:`);
    console.log(`   Cross-Origin-Embedder-Policy: require-corp`);
    console.log(`   Cross-Origin-Opener-Policy: same-origin`);
    console.log(`   Content-Type: application/wasm (for .wasm files)`);
    console.log(``);

    await server.finished;
  }
}

// CLI Interface
async function main() {
  const args = Deno.args;
  const config: ServerConfig = {
    port: 8080,
    hostname: '0.0.0.0',
    enableCORS: true,
    enableCaching: false,
    verboseLogging: false,
  };

  for (let i = 0; i < args.length; i++) {
    switch (args[i]) {
      case '--port':
      case '-p':
        config.port = parseInt(args[++i]) || 8080;
        break;
      case '--host':
      case '-h':
        config.hostname = args[++i] || '0.0.0.0';
        break;
      case '--no-cors':
        config.enableCORS = false;
        break;
      case '--cache':
        config.enableCaching = true;
        break;
      case '--verbose':
      case '-v':
        config.verboseLogging = true;
        break;
      case '--help':
        console.log(`
GTK WebGPU Demo Server

Usage: deno run --allow-read --allow-net serve-demo.ts [options]

Options:
  --port, -p <port>     Server port (default: 8080)
  --host, -h <host>     Server hostname (default: 0.0.0.0)
  --no-cors             Disable CORS headers
  --cache               Enable caching headers
  --verbose, -v         Enable verbose logging
  --help                Show this help

Examples:
  deno run --allow-read --allow-net serve-demo.ts
  deno run --allow-read --allow-net serve-demo.ts --port 3000 --verbose
  deno run --allow-read --allow-net serve-demo.ts --host localhost --cache
`);
        Deno.exit(0);
        break;
    }
  }

  const server = new StaticServer(config);
  await server.start();
}

if (import.meta.main) {
  await main();
}