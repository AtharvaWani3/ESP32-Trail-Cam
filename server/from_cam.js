const WebSocket = require('ws');
const fs = require('fs');
const { promisify } = require('util');
const { createServer } = require('http');
const sharp = require('sharp')

const is_valid_image = async(imageBytes) => {
  try {
    await sharp(Buffer.from(imageBytes)).metadata();
    return true;
  } catch (error) {
    console.log('Image invalid');
    return false;
  }
};

const handleConnection = (websocket) => {
  websocket.on('message', async (message) => {
    console.log(message.length);

    if (message.length > 5000) {
      if (is_valid_image(message)) {
        // console.log(message);
        fs.writeFileSync('image.jpg', message, 'binary');
      }
    }

    console.log();
  });

  websocket.on('close', () => {
    // Connection closed
  });
};

const server = createServer();
const wss = new WebSocket.Server({ noServer: true });

wss.on('connection', handleConnection);

server.on('upgrade', (request, socket, head) => {
  wss.handleUpgrade(request, socket, head, (websocket) => {
    wss.emit('connection', websocket, request);
  });
});

const PORT = 3001;

server.listen(PORT, () => {
  console.log(`WebSocket server is running on ws://localhost:${PORT}`);
});
