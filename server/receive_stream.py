#!/usr/bin/env python3

import asyncio
import websockets
import binascii
from io import BytesIO

from PIL import Image, UnidentifiedImageError

from pymongo import MongoClient
import gridfs

# Connect to MongoDB
client = MongoClient("mongodb://54.163.61.80:27017/ESP32_stream")
db = client.get_database("ESP32_stream")
fs = gridfs.GridFS(db)

def is_valid_image(image_bytes):
    try:
        Image.open(BytesIO(image_bytes))
        # print("image OK")
        return True
    except UnidentifiedImageError:
        print("image invalid")
        return False

async def handle_connection(websocket, path):
    while True:
        try:
            message = await websocket.recv()
            print(len(message))
            if len(message) > 5000:
                  if is_valid_image(message):
                          # Store the image in MongoDB
                          file_id = fs.put(message, filename="image.jpg")
                          if(file_id): print("img_saved_to_mongo")
                          #print(message)
                          with open("image.jpg", "wb") as f:
                                f.write(message)

            print()
        except websockets.exceptions.ConnectionClosed:
            break

async def main():
    server = await websockets.serve(handle_connection, '0.0.0.0', 3002)
    await server.wait_closed()

asyncio.run(main())
