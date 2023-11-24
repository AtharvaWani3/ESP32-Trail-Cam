from io import BytesIO
from PIL import Image
from flask import Flask, Response
from base64 import b64encode
from pymongo import MongoClient



app = Flask(__name__)

mongo_client = MongoClient('mongodb://your-mongodb-uri')  # Replace with your MongoDB URI
db = mongo_client['esp32_streams']  # Replace with your database name
collection = db['image_collection']  # Replace with your collection name

def save_to_mongodb(image_bytes):
    # Save image data to MongoDB
    image_data = {'image_bytes': image_bytes}
    collection.insert_one(image_data)
    print("Image saved to MongoDB")

@app.route('/')
def index():
    return Response(get_image(), mimetype='multipart/x-mixed-replace; boundary=frame')


def get_image():
    while True:
        try:
            with open("image.jpg", "rb") as f:
                image_bytes = f.read()

             # Save image to MongoDB
            save_to_mongodb(image_bytes)

            image = Image.open(BytesIO(image_bytes))
            img_io = BytesIO()
            image.save(img_io, 'JPEG')
            img_io.seek(0)
            img_bytes = img_io.read()
            yield (b'--frame\r\n'
                   b'Content-Type: image/jpeg\r\n\r\n' + img_bytes + b'\r\n')

        except Exception as e:
            print("encountered an exception: ")
            print(e)

            with open("placeholder.jpg", "rb") as f:
                image_bytes = f.read()

            save_to_mongodb(image_bytes)
            
            image = Image.open(BytesIO(image_bytes))
            img_io = BytesIO()
            image.save(img_io, 'JPEG')
            img_io.seek(0)
            img_bytes = img_io.read()
            yield (b'--frame\r\n'
                   b'Content-Type: image/jpeg\r\n\r\n' + img_bytes + b'\r\n')
            continue

# app.run(host='0.0.0.0', debug=False, threaded=True)

if __name__ == "__main__":
    # Use host='0.0.0.0' to make the server accessible from any device
    app.run(host='0.0.0.0', port=5000, debug=False, threaded=True)
