#pragma once

#include "openCV.h"

using namespace std;
using namespace cv;

class StereoCalibration
{
public:
	StereoCalibration(int board_width, int board_height, float square_width, float square_height);

	void InitCalibration();
	int FindChessboard(Mat &view1, Mat &view2, bool reg = false);
	bool RunCalibration();
	bool LoadCalibrationData();
	void Undistort(const Mat &view1, Mat &rview1, const Mat &view2, Mat &rview2);
	bool SaveCalibrationData();
private:
	const Size boardSize;
	const Size2f squareSize;
	bool m_calibFlag;
	vector<vector<Point2f>> m_imagePoints1, m_imagePoints2;
	Size m_imageSize;
	Mat map1, map2, map3, map4;
	cv::Mat m_cameraMatrix1;
	cv::Mat m_cameraMatrix2;
	cv::Mat m_distCoeffs1;
	cv::Mat m_distCoeffs2;
	cv::Mat m_R;
	cv::Mat m_T;
	cv::Mat m_R1;
	cv::Mat m_R2;
	cv::Mat m_P1;
	cv::Mat m_P2;
	cv::Mat m_Q;
	cv::Mat m_E, m_F;
};