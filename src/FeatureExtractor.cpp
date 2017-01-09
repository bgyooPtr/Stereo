#include "FeatureExtractor.h"

FeatureExtractor::FeatureExtractor()
	:akaze_thresh(3e-4), 
	K(cv::Matx33d(700.0, 0, 320.0, 0, 700.0, 240.0, 0, 0, 1.0)), nn_match_ratio(0.8), ransac_thresh(2.5)
{
	akaze = cv::AKAZE::create();
	akaze->setThreshold(akaze_thresh);

	fsd = cv::ximgproc::createFastLineDetector();

	matcher = cv::BFMatcher(cv::NORM_HAMMING);
}


FeatureExtractor::~FeatureExtractor()
{
}
void FeatureExtractor::pointFeatureExtracte(cv::Mat& src, std::vector<cv::KeyPoint>& kp, cv::Mat& dscr)
{
	kp.clear();
	if (src.empty())
	{
		printf("no image\n");
		return;
	}
	cv::Mat localRGB = src.clone();

	akaze->detectAndCompute(localRGB, cv::noArray(), kp, dscr);	
}

// Query, Train
void FeatureExtractor::featureMatching(std::vector<cv::KeyPoint>& kp1, std::vector<cv::KeyPoint>& kp2, cv::Mat& dscr1, cv::Mat& dscr2, double* matched_ratio)
{
	std::vector<std::vector<cv::DMatch>> matches;
	std::vector<cv::DMatch> matched_matches;
	std::vector<cv::KeyPoint> matched1, matched2;

	// Query, Train
	matcher.knnMatch(dscr1, dscr2, matches, 2);			// mather shared

	for (int i = 0; i < matches.size(); i++)
		if (matches[i][0].distance < nn_match_ratio * matches[i][1].distance)
		{
			matched1.push_back(kp1[matches[i][0].queryIdx]);
			matched2.push_back(kp2[matches[i][0].trainIdx]);
			matched_matches.push_back(matches[i][0]);
		}

	cv::Mat inlier_mask, homography;
	std::vector<cv::KeyPoint> inliers1, inliers2;
	cv::Mat dscrInlier1, dscrInlier2;
	std::vector<cv::DMatch> inlier_matches;

	if (matched1.size() >= 4)
	{
		homography = cv::findHomography(Points(matched1), Points(matched2), cv::RANSAC, ransac_thresh, inlier_mask);
	}

	if (matched1.size() < 4 || homography.empty())
	{
		std::cout << "Not matche" << std::endl;
		return;
	}

	for (int i = 0; i<matched1.size(); i++)
	{
		if (inlier_mask.at<uchar>(i))
		{
			int new_i = static_cast<int>(inliers1.size());
			inliers1.push_back(matched1[i]);
			inliers2.push_back(matched2[i]);
			dscrInlier1.push_back(dscr1.row(matched_matches[i].queryIdx));
			dscrInlier2.push_back(dscr2.row(matched_matches[i].trainIdx));
			inlier_matches.push_back(cv::DMatch(new_i, new_i, 0));
		}
	}
	m_output.m_leftKp = inliers1;
	m_output.m_rightKp = inliers2;
	m_output.m_leftDescr = dscrInlier1.clone();
	m_output.m_rightDescr = dscrInlier2.clone();

	*matched_ratio = inliers1.size() * 1.0 / matched1.size();
}

void FeatureExtractor::allCompute()		// �ʱ����
{
	std::vector<cv::KeyPoint> LKeyPt;
	std::vector<cv::KeyPoint> RKeyPt;
	cv::Mat LDesc, RDesc;

	if (m_input.m_LeftImg.empty() || m_input.m_RightImg.empty())
	{
		printf("no image\n");
		return;
	}
	cv::Mat left = m_input.m_LeftImg.clone();
	cv::Mat right = m_input.m_RightImg.clone();

	akaze->detectAndCompute(left, cv::noArray(), LKeyPt, LDesc);
	akaze->detectAndCompute(right, cv::noArray(), RKeyPt, RDesc);
	
	std::vector<std::vector<cv::DMatch>> matches;
	std::vector<cv::DMatch> matched_matches;
	std::vector<cv::KeyPoint> matched1, matched2;

	// Query, Train
	matcher.knnMatch(LDesc, RDesc, matches, 2);

	for (int i = 0; i < matches.size(); i++)
		if (matches[i][0].distance < nn_match_ratio * matches[i][1].distance)
		{
			matched1.push_back(LKeyPt[matches[i][0].queryIdx]);
			matched2.push_back(RKeyPt[matches[i][0].trainIdx]);
			matched_matches.push_back(matches[i][0]);
		}
	
	cv::Mat inlier_mask, homography;
	std::vector<cv::KeyPoint> inliers1, inliers2;
	cv::Mat dscrInlier1, dscrInlier2;
	std::vector<cv::DMatch> inlier_matches;

	if (matched1.size() >= 4)
	{
		homography = cv::findHomography(Points(matched1), Points(matched2), cv::RANSAC, ransac_thresh, inlier_mask);
	}

	if (matched1.size() < 4 || homography.empty())
	{
		std::cout << "Not matche" << std::endl;
		return;
	}

	for (int i = 0; i<matched1.size(); i++)
	{
		if (inlier_mask.at<uchar>(i))
		{
			int new_i = static_cast<int>(inliers1.size());
			inliers1.push_back(matched1[i]);
			inliers2.push_back(matched2[i]);
			dscrInlier1.push_back(LDesc.row(matched_matches[i].queryIdx));
			dscrInlier2.push_back(RDesc.row(matched_matches[i].trainIdx));
			inlier_matches.push_back(cv::DMatch(new_i, new_i, 0));
		}
	}
	m_output.m_leftKp = inliers1;
	m_output.m_rightKp = inliers2;
	m_output.m_leftDescr = LDesc;
	m_output.m_rightDescr = RDesc;

	double match_ratio = inliers1.size() * 1.0 / matched1.size();

	if (1) {
		cv::Mat ShowMatch;
		cv::hconcat(left, right, ShowMatch);
		std::vector<cv::Vec3b> colorMap = colorMapping(inliers1.size());
		m_output.m_color = colorMap;
		for (int i = 0; i < inliers1.size(); i++) {
			cv::Point pt1 = cv::Point(inliers1.at(i).pt.x + 0.5f, inliers1.at(i).pt.y + 0.5f);
			cv::Point pt2 = cv::Point(inliers2.at(i).pt.x + 640.5f, inliers2.at(i).pt.y + 0.5f);
			cv::Scalar color = cv::Scalar(colorMap.at(i).val[0], colorMap.at(i).val[1], colorMap.at(i).val[2]);
			cv::circle(ShowMatch, pt1, 3, color);
			cv::circle(ShowMatch, pt2, 3, color);
			cv::line(ShowMatch, pt1, pt2, color);
		}
		//cv::drawMatches(left, inliers1, right, inliers2, inlier_matches, ShowMatch);
		cv::imshow("Matches", ShowMatch);
		cv::waitKey(10);
	}
}
std::vector<cv::Vec3b> FeatureExtractor::colorMapping(int Size)
{
	std::vector<cv::Vec3b> dst;

	double dStep = 180.0 / (double)(Size - 1);
	for (int i = 0; i < Size; i++)
	{
		cv::Vec3b tmp;
		tmp.val[0] = (i*dStep);
		tmp.val[1] = 255;
		tmp.val[2] = 255;

		dst.push_back(tmp);
	}
	cv::cvtColor(dst, dst, CV_HSV2BGR);
	return dst;
	/*

	m_output.m_color.clear();
	for (int i = 0; i < m_output.m_leftKp.size(); i++) {
		m_output.m_color.push_back(
			cv::Scalar(
			dst.at<cv::Vec3b>((int)(m_output.m_leftKp.at(i).pt.y + 0.5f), (int)(m_output.m_leftKp.at(i).pt.x + 0.5f))[0],
			dst.at<cv::Vec3b>((int)(m_output.m_leftKp.at(i).pt.y + 0.5f), (int)(m_output.m_leftKp.at(i).pt.x + 0.5f))[1],
			dst.at<cv::Vec3b>((int)(m_output.m_leftKp.at(i).pt.y + 0.5f), (int)(m_output.m_leftKp.at(i).pt.x + 0.5f))[2])
		);
	}*/
}
void FeatureExtractor::lineCompute()
{
	std::vector<cv::Vec4f> L;

}

std::vector<cv::Point2f> FeatureExtractor::Points(std::vector<cv::KeyPoint> keyPoints)
{
	std::vector<cv::Point2f> res;
	for (unsigned i = 0; i < keyPoints.size(); i++)
		res.push_back(keyPoints[i].pt);
	return res;
}

void FeatureExtractor::run()
{
	//pointFeatureExtracte();
	std::vector<cv::KeyPoint> kp1, kp2;
	cv::Mat dscr1, dscr2;
	pointFeatureExtracte(m_input.m_LeftImg, kp1, dscr1);
	pointFeatureExtracte(m_input.m_RightImg, kp2, dscr2);

	double matched_ratio = 0;
	featureMatching(kp1, kp2, dscr1, dscr2, &matched_ratio);

	cv::Mat canvas;
	cv::hconcat(m_input.m_LeftImg, m_input.m_RightImg, canvas);
	std::vector<cv::Vec3b> colorMap = colorMapping(m_output.m_leftKp.size());
	m_output.m_color = colorMap;
	for (int i = 0; i < m_output.m_leftKp.size(); i++) {
		cv::Point pt1 = cv::Point(m_output.m_leftKp.at(i).pt.x + 0.5f, m_output.m_leftKp.at(i).pt.y + 0.5f);
		cv::Point pt2 = cv::Point(m_output.m_rightKp.at(i).pt.x + 640.5f, m_output.m_rightKp.at(i).pt.y + 0.5f);
		cv::Scalar color = cv::Scalar(colorMap.at(i).val[0], colorMap.at(i).val[1], colorMap.at(i).val[2]);
		cv::circle(canvas, pt1, 3, color);
		cv::circle(canvas, pt2, 3, color);
		cv::line(canvas, pt1, pt2, color);
	}
	//cv::drawMatches(left, inliers1, right, inliers2, inlier_matches, ShowMatch);
	cv::imshow("Matches2", canvas);
	cv::waitKey(10);

}