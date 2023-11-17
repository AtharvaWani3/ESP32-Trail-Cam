const express = require('express');
const http = require('http');
const WebSocket = require('ws');
const fs = require('fs');
const { Readable } = require('stream');
const { promisify } = require('util');
const { createCanvas, loadImage } = require('canvas');

const app = express();
const server = http.createServer(app);
const wss = new WebSocket.Server({ server });

const PORT = 3000;

app.get('/', (req, res) => {
  const stream = getStream();
  res.writeHead(200, {
    'Content-Type': 'multipart/x-mixed-replace; boundary=frame',
  });

  stream.pipe(res);

  req.on('close', () => {
    stream.destroy();
  });
});

function getStream() {
  const stream = new Readable();
  stream._read = () => {};

  async function sendImage() {
    try {
      const image = await loadImage('image.jpg');
      const canvas = createCanvas(image.width, image.height);
      const ctx = canvas.getContext('2d');
      ctx.drawImage(image, 0, 0);

      const imgBytes = canvas.toBuffer('image/jpeg');

      stream.push(
        Buffer.from(`--frame\r\nContent-Type: image/jpeg\r\n\r\n`),
      );
      stream.push(imgBytes);
      stream.push(Buffer.from(`\r\n`));
    } catch (error) {
      console.error('Encountered an exception:', error);

      const placeholder = await loadImage('placeholder.jpg');
      const canvas = createCanvas(placeholder.width, placeholder.height);
      const ctx = canvas.getContext('2d');
      ctx.drawImage(placeholder, 0, 0);

      const imgBytes = canvas.toBuffer('image/jpeg');

      stream.push(
        Buffer.from(`--frame\r\nContent-Type: image/jpeg\r\n\r\n`),
      );
      stream.push(imgBytes);
      stream.push(Buffer.from(`\r\n`));
    }
  }

  setInterval(sendImage, 1000);

  return stream;
}

wss.on('connection', (ws) => {
  ws.on('message', (message) => {
    console.log(`Received message: ${message}`);
  });
});

server.listen(PORT, () => {
  console.log(`Server is running on http://localhost:${PORT}`);
});
