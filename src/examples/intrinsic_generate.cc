#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include <iomanip>
#include <iostream>
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>

#include "camodocal/chessboard/Chessboard.h"
#include "camodocal/calib/CameraCalibration.h"
#include "../gpl/gpl.h"

int main(int argc, char** argv)
{
    cv::Size boardSize;
    float squareSize;
    std::string inputDir;
    std::string cameraModel;
    std::string cameraName;
    std::string prefix;
    std::string fileExtension;
    bool useOpenCV;
    bool viewResults;
    bool verbose;


    cameraModel = "Pinhole";
    cameraName = "Cam_0";
    
    camodocal::Camera::ModelType modelType;
    if (boost::iequals(cameraModel, "kannala-brandt"))
    {
        modelType = camodocal::Camera::KANNALA_BRANDT;
    }
    else if (boost::iequals(cameraModel, "mei"))
    {
        modelType = camodocal::Camera::MEI;
    }
    else if (boost::iequals(cameraModel, "pinhole"))
    {
        modelType = camodocal::Camera::PINHOLE;
    }
    else if (boost::iequals(cameraModel, "scaramuzza"))
    {
        modelType = camodocal::Camera::SCARAMUZZA;
    }
    else
    {
        std::cerr << "# ERROR: Unknown camera model: " << cameraModel << std::endl;
        return 1;
    }

    switch (modelType)
    {
    case camodocal::Camera::KANNALA_BRANDT:
        std::cout << "# INFO: Camera model: Kannala-Brandt" << std::endl;
        break;
    case camodocal::Camera::MEI:
        std::cout << "# INFO: Camera model: Mei" << std::endl;
        break;
    case camodocal::Camera::PINHOLE:
        std::cout << "# INFO: Camera model: Pinhole" << std::endl;
        break;
    case camodocal::Camera::SCARAMUZZA:
        std::cout << "# INFO: Camera model: Scaramuzza-Omnidirect" << std::endl;
        break;
    }

    // look for images in input directory
    std::string filename = cameraName + "_camera_calib.yaml";
    std::string m_cameraName = cameraName ;
    int m_imageWidth = 800;
    int m_imageHeight = 600;
    double m_k1 = 0, m_k2 = 0, m_p1 = 0, m_p2 = 0;
    double m_fx = 400, m_fy = 400, m_cx = 400, m_cy = 300;
    
    cv::FileStorage fs(filename, cv::FileStorage::WRITE);

    fs << "model_type" << "PINHOLE";
    fs << "camera_name" << m_cameraName;
    fs << "image_width" << m_imageWidth;
    fs << "image_height" << m_imageHeight;

    // radial distortion: k1, k2
    // tangential distortion: p1, p2
    fs << "distortion_parameters";
    fs << "{" << "k1" << m_k1
              << "k2" << m_k2
              << "p1" << m_p1
              << "p2" << m_p2 << "}";

    // projection: fx, fy, cx, cy
    fs << "projection_parameters";
    fs << "{" << "fx" << m_fx
              << "fy" << m_fy
              << "cx" << m_cx
              << "cy" << m_cy << "}";

    fs.release();
}
