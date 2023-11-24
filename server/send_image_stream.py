import asyncio
import websockets
import binascii
from io import BytesIO
from PIL import Image
from flask import Flask, Response
from base64 import b64encode
from pymongo import MongoClient
from datetime import datetime


app = Flask(__name__)
# Replace these values with your MongoDB connection details
mongo_client = MongoClient('mongodb://username:password@localhost:27017/')
database = mongo_client['your_database_name']
collection = database['your_collection_name']

@app.route('/')
def index():
    return Response(get_image(), mimetype='multipart/x-mixed-replace; boundary=frame')


def get_image():
    while True:
        try:
            with open("image.jpg", "rb") as f:
                image_bytes = f.read()
            image = Image.open(BytesIO(image_bytes))
            img_io = BytesIO()
            image.save(img_io, 'JPEG')
            img_io.seek(0)
            img_bytes = img_io.read()

            # Save image to MongoDB with a unique filename based on timestamp
            timestamp_str = datetime.now().strftime('%Y%m%d%H%M%S')
            filename = f"stream_chunk_{timestamp_str}.jpg"
            document = {
                'filename': filename,
                'timestamp': datetime.now(),
                'image_data': binascii.b2a_base64(img_bytes).decode('utf-8')  # Convert binary data to base64
            }
            collection.insert_one(document)

            # Yield the image bytes for MJPEG stream
            yield (b'--frame\r\n'
                   b'Content-Type: image/jpeg\r\n\r\n' + img_bytes + b'\r\n')

        except Exception as e:
            print("encountered an exception: ")
            print(e)

            with open("placeholder.jpg", "rb") as f:
                image_bytes = f.read()
            image = Image.open(BytesIO(image_bytes))
            img_io = BytesIO()
            image.save(img_io, 'JPEG')
            img_io.seek(0)
            img_bytes = img_io.read()

            # Save placeholder image to MongoDB with a unique filename based on timestamp
            timestamp_str = datetime.now().strftime('%Y%m%d%H%M%S')
            filename = f"placeholder_chunk_{timestamp_str}.jpg"
            document = {
                'filename': filename,
                'timestamp': datetime.now(),
                'image_data': binascii.b2a_base64(img_bytes).decode('utf-8')  # Convert binary data to base64
            }
            collection.insert_one(document)

            yield (b'--frame\r\n'
                   b'Content-Type: image/jpeg\r\n\r\n' + img_bytes + b'\r\n')
            continue

app.run(host='0.0.0.0', debug=False, threaded=True)
