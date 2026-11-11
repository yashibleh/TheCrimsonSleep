// tools/serve.mjs - a tiny static file server for THE CRIMSON SLEEP.
// Zero dependencies: uses only Node's built-in http module, and serves .wasm with
// the MIME type browsers require. Run it from the game folder:
//
//   node tools/serve.mjs          -> http://localhost:3000/
//   node tools/serve.mjs 8080     -> http://localhost:8080/
//
// Then open the printed URL in your browser.
import http from 'node:http';
import fs from 'node:fs';
import path from 'node:path';

const root = process.cwd();
const port = Number(process.argv[2] || process.env.PORT || 3000);
const host = process.env.HOST || '127.0.0.1';

const TYPES = {
  '.html': 'text/html; charset=utf-8',
  '.js': 'text/javascript; charset=utf-8',
  '.mjs': 'text/javascript; charset=utf-8',
  '.css': 'text/css; charset=utf-8',
  '.json': 'application/json; charset=utf-8',
  '.wasm': 'application/wasm',
  '.png': 'image/png',
  '.jpg': 'image/jpeg',
  '.svg': 'image/svg+xml',
  '.ico': 'image/x-icon',
  '.txt': 'text/plain; charset=utf-8',
  '.md': 'text/plain; charset=utf-8'
};

const server = http.createServer((req, res) => {
  let pathname = '/';
  try {
    pathname = decodeURIComponent(new URL(req.url, 'http://localhost').pathname);
  } catch (e) {
    res.writeHead(400, { 'content-type': 'text/plain' });
    return res.end('bad request');
  }
  if (pathname.endsWith('/')) pathname += 'index.html';

  const file = path.join(root, path.normalize(pathname));
  if (!file.startsWith(root)) {
    res.writeHead(403, { 'content-type': 'text/plain' });
    return res.end('forbidden');
  }

  fs.stat(file, (err, st) => {
    if (err || !st.isFile()) {
      res.writeHead(404, { 'content-type': 'text/plain' });
      return res.end('404 ' + pathname);
    }
    res.writeHead(200, {
      'content-type': TYPES[path.extname(file).toLowerCase()] || 'application/octet-stream',
      'content-length': st.size,
      'cache-control': 'no-cache'
    });
    if (req.method === 'HEAD') return res.end();
    fs.createReadStream(file).pipe(res);
  });
});

server.on('error', (err) => {
  if (err.code === 'EADDRINUSE') {
    console.error(`Port ${port} is already in use. Try: node tools/serve.mjs ${port + 1}`);
  } else {
    console.error(err.message);
  }
  process.exit(1);
});

server.listen(port, host, () => {
  console.log('THE CRIMSON SLEEP');
  console.log(`  serving  ${root}`);
  console.log(`  open     http://localhost:${port}/`);
  console.log('  stop     Ctrl+C');
});
