using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UI;
using System.Text;
using System.IO;

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
	public Button connectButton, disconnectButton, startRecordingButton, stopRecordingButton;

	Client client;
	WebCamTexture webCam;
	WebCamDevice[] devices;
	Color32[] image;
	Texture2D texture;
	string savePicPath;
	StreamWriter timestampWritter;

	int picid = 0;
	float currentTime;
	float beginTime = 0f;
	float imageSize = 0f;
	float lastUnlockButtonClick = 0;
	bool isConnected = false;
	bool isRecording = false;

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
		startRecordingButton.interactable = false;
		ipInputField.interactable = false;
		portInputField.interactable = false;
		connectButton.gameObject.SetActive(false);
		disconnectButton.gameObject.SetActive(true);
		isConnected = true;

	}

	public void Disconnect() {
		DisconnectByPeer();
		try {
			client.Disconnect();
		} catch (System.Exception e) {
			serverOutputText.color = failColor;
			serverOutputText.text = e.Message;
			return;
		}
		serverOutputText.color = succeedColor;
		serverOutputText.text = "Disconnected.";
	}

	private void DisconnectByPeer() {
		serverOutputText.color = succeedColor;
		serverOutputText.text = "Disconnected by peer.";
		StopSensor();
		cameraIdInputField.interactable = true;
		cameraSettingDropdown.interactable = true;
		startRecordingButton.interactable = true;
		ipInputField.interactable = true;
		portInputField.interactable = true;
		connectButton.gameObject.SetActive(true);
		disconnectButton.gameObject.SetActive(false);
		isConnected = false;
	}

	public void StartRecording() {
		StartSensor();
		serverOutputText.color = succeedColor;
		serverOutputText.text = "Recording offline ...";
		cameraIdInputField.interactable = false;
		cameraSettingDropdown.interactable = false;
		connectButton.interactable = false;
		ipInputField.interactable = false;
		portInputField.interactable = false;
		startRecordingButton.gameObject.SetActive(false);
		stopRecordingButton.gameObject.SetActive(true);

		try {
			savePicPath = Path.Combine(Application.persistentDataPath, System.DateTime.Now.ToString("yyyyMMdd_HHmmss"));
			Directory.CreateDirectory(Path.Combine(savePicPath, "pic"));
			FileInfo fi = new FileInfo(Path.Combine(savePicPath, "timestamp.txt"));
			timestampWritter = fi.CreateText();
		} catch (System.Exception e) {
			serverOutputText.color = failColor;
			serverOutputText.text = e.Message;
			return;
		}
		picid = 0;
		isRecording = true;
	}

	public void StopRecording() {
		StopSensor();
		serverOutputText.color = succeedColor;
		serverOutputText.text = "Video and timestamp are saved at " + savePicPath;
		cameraIdInputField.interactable = true;
		cameraSettingDropdown.interactable = true;
		connectButton.interactable = true;
		ipInputField.interactable = true;
		portInputField.interactable = true; 
		startRecordingButton.gameObject.SetActive(true);
		stopRecordingButton.gameObject.SetActive(false);
		timestampWritter.Close();
		timestampWritter.Dispose();
		isRecording = false;
	}

	private void StartSensor() {
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
		currentTime = Time.realtimeSinceStartup;
		sensorOutputText.color = succeedColor;
		sensorOutputText.text = "Sensor started.\n" + webCam.deviceName + ": (" + webCam.width + ", " + webCam.height + 
			")\nIMU: " + (Input.gyro.enabled ? "enabled" : "disabled") + " (currently not used)";
	}

	private void StopSensor() {
		if (webCam != null && webCam.isPlaying) webCam.Stop();
		sensorOutputText.color = succeedColor;
		sensorOutputText.text = "Sensor closed.";
		beginTime = 0f;
	}

	public void Sync() {
		if (isConnected || isRecording) {
			beginTime = Time.realtimeSinceStartup;
			lockPanel.SetActive(true);
		} else {
			sensorOutputText.color = failColor;
			sensorOutputText.text = "Failed to sync. The sensor is not connected or not recording.";
		}
	}

	public void Unlock() {
		if (Time.realtimeSinceStartup - lastUnlockButtonClick > 0.2f)
			lastUnlockButtonClick = Time.realtimeSinceStartup;
		else
			lockPanel.SetActive(false);
	}

	void Send(byte[] data, float timeStamp) {
		Debug.Log("Send b" + data.Length.ToString().PadLeft(7, '0') + timeStamp.ToString("f6").PadLeft(13, '0') + "e");
		byte[] head = Encoding.ASCII.GetBytes("b" + data.Length.ToString().PadLeft(7, '0') + timeStamp.ToString("f6").PadLeft(13, '0') + "e");   // 22 bytes
		client.GetSocket().Send(head);
		client.GetSocket().Send(data);
	}

	void Save(byte[] data, float timeStamp) {
		Debug.Log("Save pic " + timeStamp.ToString());
		File.WriteAllBytes(Path.Combine(savePicPath, "pic", (picid++).ToString() + ".jpg"), data);
		timestampWritter.WriteLine((timeStamp * 1000f).ToString());
	}

	void Start() {
		client = FindObjectOfType<Client>();
		Screen.sleepTimeout = SleepTimeout.NeverSleep;
		devices = WebCamTexture.devices;
		sensorOutputText.text = "Found " + devices.Length + " cameras.";
		lockPanel.SetActive(false);
		disconnectButton.gameObject.SetActive(false);
		stopRecordingButton.gameObject.SetActive(false);
	}

	void Update() {
		qualityText.text = ((int)qualitySlider.value).ToString();
		fpsText.text = ((int)fpsSlider.value).ToString();
		if (isConnected && !client.IsConnected()) {
			DisconnectByPeer();
		}
		if (!(isConnected || isRecording)) {
			return;
		}
		//if (Time.realtimeSinceStartup - currentTime < 1f / fpsSlider.value) return;
		try {
			Vector3 acc = Input.acceleration;
			Quaternion ori = Input.gyro.attitude;
			if (webCam.didUpdateThisFrame) {
				currentTime = Time.realtimeSinceStartup;
				webCam.GetPixels32(image);
				texture.SetPixels32(image);
				texture.Apply();
				byte[] imageBytes = texture.EncodeToJPG((int)qualitySlider.value);
				imageSize = imageBytes.Length / 1024f;
				float t = currentTime - beginTime;
				if (isConnected) {
					Send(imageBytes, t);
				}
				if (isRecording) {
					Save(imageBytes, t);
				}
				sensorText.text = "t: " + t.ToString("f1") + "\timg: " + imageSize.ToString("f1") + "kb\tacc: " + acc + "\tori: " + ori;
			}
		} catch (System.Exception e) {
			sensorOutputText.color = failColor;
			sensorOutputText.text = e.Message;
		}
	}
}
