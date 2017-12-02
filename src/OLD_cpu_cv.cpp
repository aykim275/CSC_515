#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/core.hpp>
#include <iostream>
#include <sys/time.h>
#include <sys/resource.h>
#include <stdint.h>

using namespace std;
using namespace cv;

// Structure for a Pixel
struct Pixel {
    int cluster_id;
    unsigned char r, g, b;
    unsigned int distance;
};

/*
 * Apply the mask to the image and output the result
 * */
Mat maskImg(String imPath, String mskPath) {
    Mat mask, image, result;
    
    // Read in the mask and the image given the paths
    mask = imread(mskPath);
    image = imread(imPath);
    // Grayscale the mask
    cvtColor(mask, mask, COLOR_RGB2GRAY);
    
    // Bitwise AND the mask and image
    bitwise_and(image, image, result, mask=mask);

    return result;
}


/*
 * Apply the inverted version of the mask to the image and output the result
 * */

Mat maskImgInverted(String imPath, String mskPath) {
    Mat mask, invMask, image, result;

    // Read in the mask and the image given the paths
    mask = imread(mskPath);
    image = imread(imPath);
    // Grayscale the mask
    cvtColor(mask, mask, COLOR_RGB2GRAY);
    // Invert the mask
    threshold(mask, invMask, 0, 255, 1);

    // Bitwise AND the mask and image
    bitwise_and(image, image, result, mask=invMask);
    
    return result;
}

/*
 * Create the training set using the strawberry and non-strawberry pixels
 * */
vector<Pixel> createTrainingSet() {
    vector<Pixel> trainingSet;
    Mat strawberry, nonStrawberry;
    String imFile, mskFile;

    // For every image in the training folder mask images for strawberry and non-strawberry pixels
    // and add them to the training set vector
    for (int i = 0; i < 1; i++) {
        cout << "Image " + to_string(i + 1) << endl;
        imFile = "PartA/s" + to_string(i + 10) + ".jpg";
        mskFile = "PartA_Masks/s" + to_string(i + 10) + ".jpg";
        strawberry = maskImg(imFile, mskFile);
        nonStrawberry = maskImgInverted(imFile, mskFile);
        

        // ********** STRAWBERRY PIXELS ***********
        cout << "Parsing strawberry pixels for training set." << endl;

        // split the r, g, and b channels and create new pixel object
        int channels = strawberry.channels();
        int sRows = strawberry.rows;
        int sCols = strawberry.cols * channels;

        // Check if continuous
        if (strawberry.isContinuous()) {
            sCols *= sRows;
            sRows = 1;
        }
       
        unsigned char r = 0, g = 0, b = 0; // to keep track of the rgb values
        int count = 0; // keep track of columns (b = 0; g = 1; r = 2)
        unsigned char* p = strawberry.ptr<unsigned char>();

        for (int j = 0; j < sRows; j++) {
            for (int k = 0; k < sCols; k++, p++) {
                // save the values of b, g, and r regardless of value
                switch(count) {
                    case 0:
                        b = *p;
                        break;
                    case 1:
                        g = *p;
                        break;
                    case 2:
                        r = *p;
                        break;
                    default:
                        break;
                }
               
                if (count == 2) {
                    if (r || g || b) {

                        // Create a Pixel object
                        Pixel pixel = {1, r, g, b, 0};
                        
                        // Add new pixel to the vector
                        trainingSet.push_back(pixel);
                    }
                    b = g = r = 0;
                    count = 0;
                }

                else
                    count++;
            }
        }
        
        // ********** NON-STRAWBERRY PIXELS ***********
        cout << "Parsing non-strawberry pixels for training set." << endl;

        // split the r, g, and b channels and create new pixel object
        channels = nonStrawberry.channels();
        sRows = nonStrawberry.rows;
        sCols = nonStrawberry.cols * channels;

        // Check if continuous
        if (strawberry.isContinuous()) {
            sCols *= sRows;
            sRows = 1;
        }
       
        r = 0, g = 0, b = 0; // to keep track of the rgb values
        count = 0; // keep track of columns (b = 0; g = 1; r = 2)
        p = nonStrawberry.ptr<unsigned char>();

        for (int j = 0; j < sRows; j++) {
            for (int k = 0; k < sCols; k++, p++) {
                // save the values of b, g, and r regardless of value
                switch(count) {
                    case 0:
                        b = *p;
                        break;
                    case 1:
                        g = *p;
                        break;
                    case 2:
                        r = *p;
                        break;
                    default:
                        break;
                }
               
                if (count == 2) {
                    if (r || g || b) {

                        // Create a Pixel object
                        Pixel pixel = {0, r, g, b, 0};
                        
                        // Add new pixel to the vector
                        trainingSet.push_back(pixel);
                    }
                    b = g = r = 0;
                    count = 0;
                }

                else
                    count++;
            }
        }
    }
    cout << "Training set size: " + to_string(trainingSet.size()) << endl;
    return trainingSet;
}

/*
 * Comparison between Points for sorting
 * */
bool comparison(Pixel a, Pixel b) {
    bool result = (a.distance < b.distance);
    return result;
}

/* 
 * Return the Euclidean distance between two sets of rgb values
 **/
unsigned int findDistance(unsigned int r1, unsigned int g1, unsigned int b1, 
                           unsigned int r2, unsigned int g2, unsigned int b2) {
    unsigned int distance;
    distance = sqrt((r2 - r1) * (r2 - r1) +
                    (g2 - g1) * (g2 - g1) +
                    (b2 - b1) * (b2 - b1));

    return distance;
}

/*
 * Do a comparision for the given pixel against every Pixel object in 
 * */
unsigned int classifyPixel(vector<Pixel> trainingSet, int k, 
                            unsigned int r, unsigned int g, unsigned int b) {
     
    unsigned int n = trainingSet.size();
    // Go through the training set and find the distance between the input r, g, b
    for (int i = 0; i < n; i++) {
        trainingSet[i].distance = findDistance(trainingSet[i].r, trainingSet[i].g, trainingSet[i].b, r, g, b);
    }

    // Sort the pixels by distance (shortest first)
    sort(trainingSet.begin(), trainingSet.end(), comparison);

    // Keep track of the frequencies for each classification
    int freq0 = 0;
    int freq1 = 1;
   
    for (int i = 0; i < k; i++) {
        if (trainingSet[i].cluster_id == 0)
            freq0++;
        else if (trainingSet[i].cluster_id == 1)
            freq1++;
    }

    return (freq0 > freq1 ? 0 : 1);
}


int main(int argc, char** argv) {
    Mat input;
    vector<Pixel> trainingSet;
    int channels, sRows, sCols;
    int strawberryCounter = 0;
    int resRows, resCols;

    // Read in input image to be classified
    // Check if there is an input image from the user
    if (argc < 2) {
        cout << "Must input image file" << endl;
        return -1;
    }

    input = imread(argv[1]);
    // Check if the input has valid image data
    if (!input.data) {
        cout << "Input does not hold valid data" << endl;
        return -1;
    }

    // Create the training set
    trainingSet = createTrainingSet();

    // split the r, g, and b channels
    channels = input.channels();
    sRows = input.rows;
    sCols = input.cols * channels;
    resRows = input.rows;
    resCols = input.cols;

    // Create the result matrix with the same dimensions as the input image
    Mat result(cv::Size(resCols, resRows), CV_8UC1);
    result = 0; // Initialize the matrix to all 0

    // Check if continuous
    if (input.isContinuous()) {
        sCols *= sRows;
        sRows = 1;
    }

    unsigned char r = 0, g = 0, b = 0; // to keep track of the rgb values
    int count = 0; // keep track of columns (b = 0; g = 1; r = 2)
    unsigned char* p = input.ptr<unsigned char>();
    int pixelCounter = 0;
    int colCount = 0, rowCount = 0;

    cout << sRows << endl;
    cout << sCols << endl;

    cout << resRows << endl;
    cout << resCols << endl;
    for (int j = 0; j < sRows; j++) {
        for (int k = 0; k < sCols; k++, p++) {
            // save the values of b, g, and r regardless of value
            switch(count) {
                case 0:
                    b = *p;
                    break;
                case 1:
                    g = *p;
                    break;
                case 2:
                    r = *p;
                    break;
                default:
                    break;
            }
            
            colCount = pixelCounter%resCols;
            // Get the current pixel values of the input image
            Vec3b color;
            color[0] = b;
            color[1] = g;
            color[2] = r;
            
            if (count == 2) {
                // Classify the pixel after reading the r, g, and b values
                int classification = classifyPixel(trainingSet, 5, r, g, b);
                if (classification) {
                    cout << to_string(b) + " " + to_string(g) + " " + to_string(r) << endl;
                    cout << to_string(color[0]) + " " + to_string(color[1]) + " " + to_string(color[2]) << endl;
                    // Set the pixel on the result matrix
                    result.at<Vec3b>(Point(colCount, rowCount)) = color;
                    // Print out the position of the strawberry classified pixels
                    String position = "(" + to_string(colCount) + "," + to_string(rowCount) + ")";
                    cout << "Location: " + position << endl;
                }
                
                // Reset the values
                b = g = r = 0;
                count = 0;
                pixelCounter++;
            }

            else
                count++;
            if (colCount == resCols - 1 && rowCount < resRows)
                rowCount++;
        }
    }

    imshow("Result", result);
    waitKey(0);

    return 0; 
}
