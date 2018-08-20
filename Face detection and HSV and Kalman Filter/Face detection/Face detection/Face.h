#pragma once

/**
* Created by 05 on 2018/8/20.
*/

#include <iostream>
#include <string>

#include <opencv2/core/core_c.h>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/objdetect/objdetect.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/video/tracking.hpp>

using namespace std;
using namespace cv;

namespace Face_detection {

	using namespace System;
	using namespace System::ComponentModel;
	using namespace System::Collections;
	using namespace System::Windows::Forms;
	using namespace System::Data;
	using namespace System::Drawing;

	/// <summary>
	/// Summary for MyForm
	/// </summary>
	public ref class MyForm : public System::Windows::Forms::Form
	{
	public:
		MyForm(void)
		{
			InitializeComponent();
			//
			//TODO: Add the constructor code here
			//
		}

	protected:
		/// <summary>
		/// Clean up any resources being used.
		/// </summary>
		~MyForm()
		{
			if (components)
			{
				delete components;
			}
		}
	private: System::Windows::Forms::Button^  btn_detection;
	protected:

	protected:

	private:
		/// <summary>
		/// Required designer variable.
		/// </summary>
		System::ComponentModel::Container ^components;

#pragma region Windows Form Designer generated code
		/// <summary>
		/// Required method for Designer support - do not modify
		/// the contents of this method with the code editor.
		/// </summary>
		void InitializeComponent(void)
		{
			this->btn_detection = (gcnew System::Windows::Forms::Button());
			this->SuspendLayout();
			// 
			// btn_detection
			// 
			this->btn_detection->Location = System::Drawing::Point(99, 194);
			this->btn_detection->Name = L"btn_detection";
			this->btn_detection->Size = System::Drawing::Size(75, 23);
			this->btn_detection->TabIndex = 0;
			this->btn_detection->Text = L"Detection";
			this->btn_detection->UseVisualStyleBackColor = true;
			this->btn_detection->Click += gcnew System::EventHandler(this, &MyForm::btn_detection_Click);
			// 
			// MyForm
			// 
			this->AutoScaleDimensions = System::Drawing::SizeF(6, 12);
			this->AutoScaleMode = System::Windows::Forms::AutoScaleMode::Font;
			this->ClientSize = System::Drawing::Size(284, 261);
			this->Controls->Add(this->btn_detection);
			this->Name = L"MyForm";
			this->Text = L"MyForm";
			this->ResumeLayout(false);

		}
#pragma endregion
	private: System::Void btn_detection_Click(System::Object^  sender, System::EventArgs^  e) {

		Mat frame;
		Mat grayFrame;
		VideoCapture capture(0);
		string cascadeName = "haarcascade_frontalface_alt2.xml";
		CascadeClassifier face_cascade;
		cv::Point predictPt;
		cv::Point pt1;
		cv::Point pt2;

		Mat prehsvFrame;
		Mat curhsvFrame;
		float prehf = 0.0;
		float curhf = 0.0;
		int tpt1 = 0;
		int tpt2 = 0;
		int count = 0;
		int frameCount = 0;
		int max = 0;
		int fflag = 0;

		Boolean contr = false;

		if (!capture.isOpened()) {
			cout << "Error loading video" << endl;
			return;
		}
		if (!face_cascade.load(cascadeName)) {
			cout << "Error loading cascade file" << endl;
			return;
		}

		const int stateNum = 4;
		const int measureNum = 2;
		KalmanFilter KF(stateNum, measureNum, 0);       //state(x, y, detaX, detaY) 測量座標XY，總共4個
		Mat measurement = Mat::zeros(measureNum, 1, CV_32F); //measurement(x, y)
		KF.transitionMatrix = (Mat_<float>(4, 4) << 1, 0, 1, 0, 0, 1, 0, 1, 0, 0, 1, 0, 0, 0, 0, 1); //transition matrix [1, 0, 1, 0 ; 0, 1, 0, 1 ; 0, 0, 1, 0 ; 0, 0, 0, 1]

		//將以下矩陣設置成對角陣
		setIdentity(KF.measurementMatrix);
		setIdentity(KF.processNoiseCov, Scalar::all(1e-5));
		setIdentity(KF.measurementNoiseCov, Scalar::all(1e-1));
		setIdentity(KF.errorCovPost, Scalar::all(1));

		//選定視訊裝置，0為自動偵測
		while (true) {
			//擷取視訊
			capture.read(frame);
			//轉換灰階
			grayFrame = Mat(frame.size(), CV_8U);
			cvtColor(frame, grayFrame, CV_BGR2GRAY);

			vector<Rect> face;
			face_cascade.detectMultiScale(grayFrame, face, 1.1, 3, 0 | CV_HAAR_SCALE_IMAGE, cv::Size(10, 10));

			for (size_t i = 0; i < face.size(); i++) {

				if ((face[i].height + face[i].width) > max) {
					max = face[i].height + face[i].width;
					fflag = i;
				}

			}

			if (count >= 0) {

				for (size_t i = fflag; i < face.size() && i < fflag + 1; i++) {

					pt1 = cv::Point(face[i].x + face[i].width, face[i].y + face[i].height);
					pt2 = cv::Point(face[i].x, face[i].y);

					frame(Rect(face[i].x, face[i].y, pt1.x - pt2.x, pt1.y - pt2.y)).copyTo(prehsvFrame);
					cvtColor(prehsvFrame, prehsvFrame, CV_BGR2HSV);
					extractChannel(prehsvFrame, prehsvFrame, 0);



					prehf = 0;
					prehf = sum(prehsvFrame).val[0];
					count = 0;

					//rectangle(frame, pt1, pt2, Scalar(0, 0, 255), 2, 8, 0);

				}
			}
			max = 0;
			fflag = 0;
			count++;

			if (!curhsvFrame.empty()) {
				imshow("h", prehsvFrame);
				imshow("s", curhsvFrame);
			}

			if (face.empty()) {
				
				if (!prehsvFrame.empty() /*&& contr*/) {

					//卡曼濾波預測
					Mat prediction = KF.predict(); //計算預測值，返回x',y'
					predictPt = cv::Point((int)prediction.at<float>(0), (int)prediction.at<float>(1));

					if (predictPt.x > 0 && predictPt.y > 0 && (predictPt.x + prehsvFrame.rows) < frame.cols && (predictPt.y + prehsvFrame.cols) < frame.rows) {

						frame(Rect(predictPt.x, predictPt.y, prehsvFrame.rows, prehsvFrame.cols)).copyTo(curhsvFrame);

						cvtColor(curhsvFrame, curhsvFrame, CV_BGR2HSV);


						extractChannel(curhsvFrame, curhsvFrame, 0);
						absdiff(curhsvFrame, prehsvFrame, curhsvFrame);


						curhf = 0;
						if ((sum(curhsvFrame).val[0] / curhsvFrame.total()) >= 40) {

							//更新測量座標
							measurement.at<float>(0) = (float)predictPt.x;
							measurement.at<float>(1) = (float)predictPt.y;

							//更新
							KF.correct(measurement);
							rectangle(frame, predictPt, cv::Point(predictPt.x + prehsvFrame.rows, predictPt.y + prehsvFrame.cols), Scalar(0, 255, 0), 2, 8, 0);
							//circle(frame, predictPt, 5, CV_RGB(0, 255, 0), 3);
						}
						else {
							//更新測量座標
							measurement.at<float>(0) = (float)pt2.x;
							measurement.at<float>(1) = (float)pt2.y;

							//更新
							KF.correct(measurement);
							rectangle(frame, predictPt, cv::Point(predictPt.x + prehsvFrame.rows, predictPt.y + prehsvFrame.cols), Scalar(0, 255, 0), 2, 8, 0);
							//circle(frame, predictPt, 5, CV_RGB(0, 255, 0), 3);
						}
					}
				}
			}
			else {
				//卡曼濾波預測
				Mat prediction = KF.predict(); //計算預測值，返回x',y'
				predictPt = cv::Point((int)prediction.at<float>(0), (int)prediction.at<float>(1));

				//更新測量座標
				measurement.at<float>(0) = (float)pt2.x;
				measurement.at<float>(1) = (float)pt2.y;

				//更新
				KF.correct(measurement);
				rectangle(frame, predictPt, cv::Point(predictPt.x + pt1.x - pt2.x, predictPt.y + pt1.y - pt2.y), Scalar(0, 255, 0), 2, 8, 0);
				//circle(frame, predictPt, 5, CV_RGB(0, 255, 0), 3);

			}
			
			imshow("window", frame);
			int loh = 0;
			if (loh == 0) {
				loh++;
			}

			if (waitKey(33) >= 0)
			{
				imwrite("123.jpg", curhsvFrame);
				imwrite("234.jpg", prehsvFrame);
				imwrite("345.jpg", frame(Rect(pt2.x, pt2.y, prehsvFrame.rows, prehsvFrame.cols)));
				imwrite("456.jpg", frame);
				break;
			}
		}
		capture.release();
	}
	};
}
