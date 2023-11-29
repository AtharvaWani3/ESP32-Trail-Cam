# ESP32-Trail-Cam

<h2>Build of materials</h2>
<ul>
<li>TTGo T-Camera</li>
<li>Long range Antenna</li>
<li>5V Li-ion battery or Power Bank</li>
</ul>

<h2>Setup</h2>
<ol>
<li>Start EC2 instance with ubuntu.</li>
<li>Open the ports 3002, 5000 to recieve traffic from any IP. Open port 27017 to "myIP"</li>
<li>Clone the repository to the EC2 instance.</li>
<li>Navigate inside the repository to the folder "server".
    <br>Add executable tags to following files:"dependencies.sh", "receive_stream.py", "send_image_stream.py" by using the following command:
    <br> "chmod +x &lt;file_name&gt;"</li>
<li>Run the command "./dependencies.sh"</li>
<li>Start the Mongod service.</li>
<li>Change the bindIP for mongodb using the command "sudo nano /etc/mongod.conf". Restart Mongod service.</li>
<li>Open the Arduino IDE and open "camera.ino".</li>
<li>Enter the SSID and password for the wifi network.</li>
<li>Change the Websocket server IP to EC2 instance's public IP.</li>
<li>Optionally change the stream time.</li>
</ol>

<h2>Use the ESP32-Trail camera</h2>
<ol>
<li>Open the same EC2 instance in two separate tabs of the browser by duplicating the first tab.</li>
<li>Navigate to the server folder under the repository folder in both tabs. You DO NOT NEED to re-clone the repository in the second tab.</li>
<li>In the first tab, run the command "python3 receive_stream.py"</li>
<li>In the second tab, run the command "python3 send_image_stream.py"</li>
<li>Upload the "camera.ino" code on to the TTGO T-Camera.</li>
<li>To access the stream, connect to &lt;EC2_IP&gt;:5000.</li>
<li>All of the stream data is also stored on Mongodb and can be accessed using MongoDB Compass.</li>
</ol>

<h2>Future Development</h2>
<ul>
<li>Upgrade the sensor stack to add point lidar and object detection to classify the streams into folders dedicated to different animals.</li>
<li>Add IR Flash for improved low light image quality.</li>
<li>Allow access to the old streams on the website.</li>
</ul>
