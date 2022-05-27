using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UI;
using System.Text;

public class Sensor : MonoBehaviour
{
	public InputField ipInputField, portInputField;
	public Text serverOutputText;
	public InputField cameraIdInputField;
	public Dropdown cameraSettingDropdown;
	public Slider qualitySlider, fpsSlider;
	public Text qualityText, fpsText;
	public RawImage cameraRawImage;
	public Text sensorText;
	public Text sensorOutputText;
	public GameObject lockPanel;

	Client client;
	WebCamTexture webCam;
	WebCamDevice[] devices;
	Color32[] image;
	Texture2D texture;

	float currentTime;
	float beginTime = 0f;
	float imageSize = 0f;
	float lastUnlockButtonClick = 0;
	bool isConnected = false;

	Color failColor = new Color(1, 0, 0);
	Color succeedColor = new Color(0, 0.5f, 0);


	public void Connect() {
		try {
			client.serverIp = ipInputField.text;
			client.port = int.Parse(portInputField.text);
			client.Connect();
		} catch (System.Exception e) {
			serverOutputText.color = failColor;
			serverOutputText.text = e.Message;
			return;
		}
		StartSensor();
		serverOutputText.color = succeedColor;
		serverOutputText.text = "Connected.";
		cameraIdInputField.interactable = false;
		cameraSettingDropdown.interactable = false;
		isConnected = true;
	}

	public void Disconnect() {
		try {
			client.Disconnect();
		} catch (System.Exception e) {
			serverOutputText.color = failColor;
			serverOutputText.text = e.Message;
			return;
		}
		if (webCam != null && webCam.isPlaying) webCam.Stop();
		cameraIdInputField.interactable = true;
		cameraSettingDropdown.interactable = true;
		serverOutputText.color = succeedColor;
		serverOutputText.text = "Disconnected.";
		sensorOutputText.color = succeedColor;
		sensorOutputText.text = "Sensor closed.";
		beginTime = 0f;
		isConnected = false;
	}

	private void DisconnectByPeer() {
		if (webCam != null && webCam.isPlaying) webCam.Stop();
		cameraIdInputField.interactable = true;
		cameraSettingDropdown.interactable = true;
		serverOutputText.color = succeedColor;
		serverOutputText.text = "Disconnected by peer.";
		beginTime = 0f;
		isConnected = false;
	}


	void Start() {
		client = FindObjectOfType<Client>();
		Screen.sleepTimeout = SleepTimeout.NeverSleep;
		devices = WebCamTexture.devices;
		sensorOutputText.text = "Found " + devices.Length + " cameras.";
		lockPanel.SetActive(false);
	}

	public void StartSensor() {
		try {
			Input.gyro.enabled = true;
			string deviceName = devices[int.Parse(cameraIdInputField.text)].name;
			switch (cameraSettingDropdown.value) {
				case 0: webCam = new WebCamTexture(deviceName, 640, 480, 60); break;
				case 1: webCam = new WebCamTexture(deviceName, 640, 480, 30); break;
				case 2: webCam = new WebCamTexture(deviceName, 1280, 720, 60); break;
				case 3: webCam = new WebCamTexture(deviceName, 1280, 720, 30); break;
				case 4: webCam = new WebCamTexture(deviceName, 1920, 1080, 60); break;
				case 5: webCam = new WebCamTexture(deviceName, 1920, 1080, 30); break;
			}
			
			cameraRawImage.texture = webCam;
			webCam.autoFocusPoint = new Vector2(0.5f, 0.5f);
			webCam.Play();
			image = new Color32[webCam.width * webCam.height];
			texture = new Texture2D(webCam.width, webCam.height, TextureFormat.ARGB32, false);
		} catch (System.Exception e) {
			sensorOutputText.color = failColor;
			sensorOutputText.text = e.Message;
			return;
		}
		currentTime = Time.time;
		sensorOutputText.color = succeedColor;
		sensorOutputText.text = "Sensor started.\n" + webCam.deviceName + ": (" + webCam.width + ", " + webCam.height + 
			")\nIMU: " + (Input.gyro.enabled ? "enabled" : "disabled") + " (currently not used)";
	}

	public void SetBeginTime() {
		if (isConnected) {
			beginTime = Time.time;
			lockPanel.SetActive(true);
		} else {
			sensorOutputText.color = failColor;
			sensorOutputText.text = "Failed to sync. The sensor is not connected.";
		}
	}

	public void Unlock() {
		if (Time.time - lastUnlockButtonClick > 0.2f)
			lastUnlockButtonClick = Time.time;
		else
			lockPanel.SetActive(false);
	}

	void Send(byte[] data, float timeStamp) {
		if (!isConnected) return;
		Debug.Log("b" + data.Length.ToString().PadLeft(7, '0') + timeStamp.ToString("f6").PadLeft(13, '0') + "e");
		byte[] head = Encoding.ASCII.GetBytes("b" + data.Length.ToString().PadLeft(7, '0') + timeStamp.ToString("f6").PadLeft(13, '0') + "e");   // 22 bytes
		client.GetSocket().Send(head);
		client.GetSocket().Send(data);
	}

	void Update() {
		qualityText.text = ((int)qualitySlider.value).ToString();
		fpsText.text = ((int)fpsSlider.value).ToString();
		if (!client.IsConnected()) {    // one side has closed the connection
			if (isConnected) {
				DisconnectByPeer();
			}
			return;
		}

		//if (Time.time - currentTime < 1f / fpsSlider.value) return;
		Vector3 acc = Input.acceleration;
		Quaternion ori = Input.gyro.attitude;

		try {
			if (webCam.didUpdateThisFrame) {
				currentTime = Time.time;
				webCam.GetPixels32(image);
				texture.SetPixels32(image);
				texture.Apply();
				byte[] imageBytes = texture.EncodeToJPG((int)qualitySlider.value);
				imageSize = imageBytes.Length / 1024f;
				Send(imageBytes, currentTime - beginTime);
			}
			sensorText.text = "img: " + imageSize.ToString("f1") + "kb\tacc: " + acc + "\tori: " + ori;
		} catch (System.Exception e) {
			sensorOutputText.color = failColor;
			sensorOutputText.text = e.Message;
		}
	}
}
