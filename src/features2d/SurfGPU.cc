#include "SurfGPU.h"

#include <iostream>

namespace camodocal
{

cv::Ptr<SurfGPU> SurfGPU::mInstance;
boost::mutex SurfGPU::mInstanceMutex;

SurfGPU::SurfGPU(double hessianThreshold, int nOctaves,
                 int nOctaveLayers, bool extended,
                 float keypointsRatio)
 : mSURF_GPU(hessianThreshold, nOctaves, nOctaveLayers, extended, keypointsRatio)
{

}

SurfGPU::~SurfGPU()
{
    mImageGPU.release();
    mMaskGPU.release();
    mKptsGPU.release();
    mDtorsGPU.release();
    mMatchMaskGPU.release();
    mQDtorsGPU.release();
    mTDtorsGPU.release();
}

cv::Ptr<SurfGPU>
SurfGPU::instance(double hessianThreshold, int nOctaves,
                  int nOctaveLayers, bool extended,
                  float keypointsRatio)
{
    boost::mutex::scoped_lock lock(mInstanceMutex);

    if (mInstance.empty())
    {
        mInstance = cv::Ptr<SurfGPU>(new SurfGPU(hessianThreshold, nOctaves, nOctaveLayers, extended, keypointsRatio));
    }

    return mInstance;
}

void
SurfGPU::detect(const cv::Mat& image, std::vector<cv::KeyPoint>& keypoints,
                const cv::Mat& mask)
{
    boost::mutex::scoped_lock lock(mSURFMutex);

    mImageGPU.upload(image);
    if (!mask.empty())
    {
        mMaskGPU.upload(mask);
    }
    else
    {
        mMaskGPU.release();
    }

    try
    {
        mSURF_GPU(mImageGPU, mMaskGPU, mKptsGPU);

        mSURF_GPU.downloadKeypoints(mKptsGPU, keypoints);
    }
    catch (cv::Exception& exception)
    {
        std::cout << "# ERROR: Surf GPU feature detection failed: " << exception.msg << std::endl;
    }
}

void
SurfGPU::compute(const cv::Mat& image,
                 std::vector<cv::KeyPoint>& keypoints,
                 cv::Mat& descriptors)
{
    boost::mutex::scoped_lock lock(mSURFMutex);

    mImageGPU.upload(image);

    try
    {
        mSURF_GPU(mImageGPU, cv::gpu::GpuMat(), keypoints, mDtorsGPU, true);
    }
    catch (cv::Exception& exception)
    {
        std::cout << "# ERROR: Surf GPU descriptor computation failed: " << exception.msg << std::endl;
    }

    mDtorsGPU.download(descriptors);
}

void
SurfGPU::knnMatch(const cv::Mat& queryDescriptors,
                  const cv::Mat& trainDescriptors,
                  std::vector<std::vector<cv::DMatch> >& matches,
                  int k,
                  const cv::Mat& mask,
                  bool compactResult)
{
    boost::mutex::scoped_lock lock(mMatchMutex);

    if (queryDescriptors.empty() || trainDescriptors.empty())
    {
        matches.clear();
        return;
    }

    matches.reserve(queryDescriptors.rows);

    mQDtorsGPU.upload(queryDescriptors);
    mTDtorsGPU.upload(trainDescriptors);

    if (!mask.empty())
    {
        mMatchMaskGPU.upload(mask);
    }
    else
    {
        mMatchMaskGPU.release();
    }

    mMatcher.knnMatch(mQDtorsGPU, mTDtorsGPU, matches, k, mMatchMaskGPU, compactResult);
}

void
SurfGPU::radiusMatch(const cv::Mat& queryDescriptors,
                     const cv::Mat& trainDescriptors,
                     std::vector<std::vector<cv::DMatch> >& matches,
                     float maxDistance,
                     const cv::Mat& mask,
                     bool compactResult)
{
    boost::mutex::scoped_lock lock(mMatchMutex);

    if (queryDescriptors.empty() || trainDescriptors.empty())
    {
        matches.clear();
        return;
    }

    matches.reserve(queryDescriptors.rows);

    mQDtorsGPU.upload(queryDescriptors);
    mTDtorsGPU.upload(trainDescriptors);

    if (!mask.empty())
    {
        mMatchMaskGPU.upload(mask);
    }
    else
    {
        mMatchMaskGPU.release();
    }

    mMatcher.radiusMatch(mQDtorsGPU, mTDtorsGPU, matches, maxDistance, mMatchMaskGPU, compactResult);
}

void
SurfGPU::match(const cv::Mat& image1, std::vector<cv::KeyPoint>& keypoints1,
               const cv::Mat& mask1,
               const cv::Mat& image2, std::vector<cv::KeyPoint>& keypoints2,
               const cv::Mat& mask2,
               std::vector<cv::DMatch>& matches,
               bool useProvidedKeypoints)
{
    boost::mutex::scoped_lock lock(mSURFMutex);

    cv::gpu::GpuMat imageGPU[2];
    cv::gpu::GpuMat maskGPU[2];
    cv::gpu::GpuMat dtorsGPU[2];

    imageGPU[0].upload(image1);
    imageGPU[1].upload(image2);

    if (!mask1.empty())
    {
        maskGPU[0].upload(mask1);
    }
    if (!mask2.empty())
    {
        maskGPU[1].upload(mask2);
    }

    try
    {
        mSURF_GPU(imageGPU[0], maskGPU[0], keypoints1, dtorsGPU[0], useProvidedKeypoints);
        mSURF_GPU(imageGPU[1], maskGPU[1], keypoints2, dtorsGPU[1], useProvidedKeypoints);

        std::vector<cv::DMatch> fwdMatches;
        mMatcher.match(dtorsGPU[0], dtorsGPU[1], fwdMatches);

        std::vector<cv::DMatch> revMatches;
        mMatcher.match(dtorsGPU[1], dtorsGPU[0], revMatches);

        // cross-check
        matches.clear();
        for (size_t i = 0; i < fwdMatches.size(); ++i)
        {
            cv::DMatch& fwdMatch = fwdMatches.at(i);
            cv::DMatch& revMatch = revMatches.at(fwdMatch.trainIdx);

            if (fwdMatch.queryIdx == revMatch.trainIdx &&
                fwdMatch.trainIdx == revMatch.queryIdx)
            {
                matches.push_back(fwdMatch);
            }
        }
    }
    catch (cv::Exception& exception)
    {
        std::cout << "# ERROR: Surf GPU image matching failed: " << exception.msg << std::endl;
    }
}

}