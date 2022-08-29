
#include <iostream>
#include <vector>
#include <fstream>
#include <cmath>
using namespace std;


// Pixel structure
struct Pixel
{
    // Red, green, blue color values
    int red;
    int green;
    int blue;
};

/**
 * Gets an integer from a binary stream.
 * Helper function for read_image()
 * @param stream the stream
 * @param offset the offset at which to read the integer
 * @param bytes  the number of bytes to read
 * @return the integer starting at the given offset
 */ 
int get_int(fstream& stream, int offset, int bytes)
{
    stream.seekg(offset);
    int result = 0;
    int base = 1;
    for (int i = 0; i < bytes; i++)
    {   
        result = result + stream.get() * base;
        base = base * 256;
    }
    return result;
}

/**
 * Reads the BMP image specified and returns the resulting image as a vector
 * @param filename BMP image filename
 * @return the image as a vector of vector of Pixels
 */
vector<vector<Pixel>> read_image(string filename)
{
    // Open the binary file
    fstream stream;
    stream.open(filename, ios::in | ios::binary);

    // Get the image properties
    int file_size = get_int(stream, 2, 4);
    int start = get_int(stream, 10, 4);
    int width = get_int(stream, 18, 4);
    int height = get_int(stream, 22, 4);
    int bits_per_pixel = get_int(stream, 28, 2);

    // Scan lines must occupy multiples of four bytes
    int scanline_size = width * (bits_per_pixel / 8);
    int padding = 0;
    if (scanline_size % 4 != 0)
    {
        padding = 4 - scanline_size % 4;
    }

    // Return empty vector if this is not a valid image
    if (file_size != start + (scanline_size + padding) * height)
    {
        return {};
    }

    // Create a vector the size of the input image
    vector<vector<Pixel>> image(height, vector<Pixel> (width));

    int pos = start;
    // For each row, starting from the last row to the first
    // Note: BMP files store pixels from bottom to top
    for (int i = height - 1; i >= 0; i--)
    {
        // For each column
        for (int j = 0; j < width; j++)
        {
            // Go to the pixel position
            stream.seekg(pos);

            // Save the pixel values to the image vector
            // Note: BMP files store pixels in blue, green, red order
            image[i][j].blue = stream.get();
            image[i][j].green = stream.get();
            image[i][j].red = stream.get();

            // We are ignoring the alpha channel if there is one

            // Advance the position to the next pixel
            pos = pos + (bits_per_pixel / 8);
        }

        // Skip the padding at the end of each row
        stream.seekg(padding, ios::cur);
        pos = pos + padding;
    }

    // Close the stream and return the image vector
    stream.close();
    return image;
}

/**
 * Sets a value to the char array starting at the offset using the size
 * specified by the bytes.
 * This is a helper function for write_image()
 * @param arr    Array to set values for
 * @param offset Starting index offset
 * @param bytes  Number of bytes to set
 * @param value  Value to set
 * @return nothing
 */
void set_bytes(unsigned char arr[], int offset, int bytes, int value)
{
    for (int i = 0; i < bytes; i++)
    {
        arr[offset+i] = (unsigned char)(value>>(i*8));
    }
}

/**
 * Write the input image to a BMP file name specified
 * @param filename The BMP file name to save the image to
 * @param image    The input image to save
 * @return True if successful and false otherwise
 */
bool write_image(string filename, const vector<vector<Pixel>>& image)
{
    // Get the image width and height in pixels
    int width_pixels = image[0].size();
    int height_pixels = image.size();

    // Calculate the width in bytes incorporating padding (4 byte alignment)
    int width_bytes = width_pixels * 3;
    int padding_bytes = 0;
    padding_bytes = (4 - width_bytes % 4) % 4;
    width_bytes = width_bytes + padding_bytes;

    // Pixel array size in bytes, including padding
    int array_bytes = width_bytes * height_pixels;

    // Open a file stream for writing to a binary file
    fstream stream;
    stream.open(filename, ios::out | ios::binary);

    // If there was a problem opening the file, return false
    if (!stream.is_open())
    {
        return false;
    }

    // Create the BMP and DIB Headers
    const int BMP_HEADER_SIZE = 14;
    const int DIB_HEADER_SIZE = 40;
    unsigned char bmp_header[BMP_HEADER_SIZE] = {0};
    unsigned char dib_header[DIB_HEADER_SIZE] = {0};

    // BMP Header
    set_bytes(bmp_header,  0, 1, 'B');              // ID field
    set_bytes(bmp_header,  1, 1, 'M');              // ID field
    set_bytes(bmp_header,  2, 4, BMP_HEADER_SIZE+DIB_HEADER_SIZE+array_bytes); // Size of BMP file
    set_bytes(bmp_header,  6, 2, 0);                // Reserved
    set_bytes(bmp_header,  8, 2, 0);                // Reserved
    set_bytes(bmp_header, 10, 4, BMP_HEADER_SIZE+DIB_HEADER_SIZE); // Pixel array offset

    // DIB Header
    set_bytes(dib_header,  0, 4, DIB_HEADER_SIZE);  // DIB header size
    set_bytes(dib_header,  4, 4, width_pixels);     // Width of bitmap in pixels
    set_bytes(dib_header,  8, 4, height_pixels);    // Height of bitmap in pixels
    set_bytes(dib_header, 12, 2, 1);                // Number of color planes
    set_bytes(dib_header, 14, 2, 24);               // Number of bits per pixel
    set_bytes(dib_header, 16, 4, 0);                // Compression method (0=BI_RGB)
    set_bytes(dib_header, 20, 4, array_bytes);      // Size of raw bitmap data (including padding)                     
    set_bytes(dib_header, 24, 4, 2835);             // Print resolution of image (2835 pixels/meter)
    set_bytes(dib_header, 28, 4, 2835);             // Print resolution of image (2835 pixels/meter)
    set_bytes(dib_header, 32, 4, 0);                // Number of colors in palette
    set_bytes(dib_header, 36, 4, 0);                // Number of important colors

    // Write the BMP and DIB Headers to the file
    stream.write((char*)bmp_header, sizeof(bmp_header));
    stream.write((char*)dib_header, sizeof(dib_header));

    // Initialize pixel and padding
    unsigned char pixel[3] = {0};
    unsigned char padding[3] = {0};

    // Pixel Array (Left to right, bottom to top, with padding)
    for (int h = height_pixels - 1; h >= 0; h--)
    {
        for (int w = 0; w < width_pixels; w++)
        {
            // Write the pixel (Blue, Green, Red)
            pixel[0] = image[h][w].blue;
            pixel[1] = image[h][w].green;
            pixel[2] = image[h][w].red;
            stream.write((char*)pixel, 3);
        }
        // Write the padding bytes
        stream.write((char *)padding, padding_bytes);
    }

    // Close the stream and return true
    stream.close();
    return true;
}

// PROCESS 1 - Adds vignette effect - dark corners
vector<vector<Pixel>> process_1(const vector<vector<Pixel>>& image)
{
    int num_rows = image.size(); //Gets the number of rows (i.e. height) in a 2D vector named image
    int num_columns = image[0].size(); //Gets the number of columns (i.e. width) in a 2D vector named image
    vector<vector<Pixel>> new_image(num_rows, vector<Pixel> (num_columns)); //define a new 2D vector with the Pixel values and set it to have the same rows and columns as the original image. 
    for (int row = 0; row < num_rows; row++)
    {
        for (int col = 0; col < num_columns; col++)
        {
            //find the distance from each cell to the center and calculate scaling factor
            double distance = sqrt(pow(col - num_columns/2,2) + pow(row - num_rows/2,2));
            double scaling_factor = (num_rows - distance)/num_rows;
            int newred = image[row][col].red*scaling_factor;
            int newgreen = image[row][col].green*scaling_factor;
            int newblue = image[row][col].blue*scaling_factor;
            //Save the new color values to the corresponding pixel located at this row and column in the new 2D vector
            new_image[row][col].red = newred;
            new_image[row][col].green = newgreen;
            new_image[row][col].blue = newblue;
        }
    }
    return new_image;
}
// PROCESS 2 - Adds clarendon type effect - darks darker and lights lighter
vector<vector<Pixel>> process_2(const vector<vector<Pixel>>& image, double scaling_factor)
{
    int num_rows = image.size(); //Gets the number of rows (i.e. height) in a 2D vector named image
    int num_columns = image[0].size(); //Gets the number of columns (i.e. width) in a 2D vector named image
    vector<vector<Pixel>> new_image(num_rows, vector<Pixel> (num_columns)); //define a new 2D vector with the Pixel values and set it to have the same rows and columns as the original image. 
    for (int row = 0; row < num_rows; row++)
    {
        for (int col = 0; col < num_columns; col++)
        {
            //What are the red, blue and green values for current cell? Obtain the info and calculate the average
            int red_value = image[row][col].red;
            int green_value = image[row][col].green;
            int blue_value = image[row][col].blue;
            double average_value = (red_value + green_value + blue_value)/3;
            //If the cell is light, make it lighter
            if (average_value >= 170)
            {
                int new_red = 255 - (255 - red_value)* scaling_factor;
                int new_green = 255 - (255 - green_value)* scaling_factor;
                int new_blue = 255 - (255 - blue_value)* scaling_factor;
                //Save the new color values to the corresponding pixel located at this row and column in the new 2D vector
                new_image[row][col].red = new_red;
                new_image[row][col].green = new_green;
                new_image[row][col].blue = new_blue;
            }
            //If the cell is dark, make it darker
            else if (average_value < 90)
            {
                int new_red = red_value* scaling_factor;
                int new_green = green_value* scaling_factor;
                int new_blue = blue_value* scaling_factor;
                //Save the new color values to the corresponding pixel located at this row and column in the new 2D vector
                new_image[row][col].red = new_red;
                new_image[row][col].green = new_green;
                new_image[row][col].blue = new_blue;
            }
            else
            {
                int new_red = red_value;
                int new_green = green_value;
                int new_blue = blue_value;
                //Save the new color values to the corresponding pixel located at this row and column in the new 2D vector
                new_image[row][col].red = new_red;
                new_image[row][col].green = new_green;
                new_image[row][col].blue = new_blue;
            }
        }
    }
    return new_image;
}

//PROCESS 3 - Greyscale
vector<vector<Pixel>> process_3(const vector<vector<Pixel>>& image)
{
    int num_rows = image.size(); //Gets the number of rows (i.e. height) in a 2D vector named image
    int num_columns = image[0].size(); //Gets the number of columns (i.e. width) in a 2D vector named image
    vector<vector<Pixel>> new_image(num_rows, vector<Pixel> (num_columns)); //define a new 2D vector with the Pixel values and set it to have the same rows and columns as the original image. 
    for (int row = 0; row < num_rows; row++)
    {
        for (int col = 0; col < num_columns; col++)
        {
            //Obtaining the red, blue and green values for current cell in original image 
            int red_value = image[row][col].red;
            int green_value = image[row][col].green;
            int blue_value = image[row][col].blue;
            double gray_value = (red_value + green_value + blue_value)/3;
            //Save the new color values to the corresponding pixel located at this row and column in the new 2D vector
            new_image[row][col].red = gray_value;
            new_image[row][col].green = gray_value;
            new_image[row][col].blue = gray_value;
        }
    }
    return new_image;
}

//PROCESS 4 - Rotate by 90 degrees
vector<vector<Pixel>> process_4(const vector<vector<Pixel>>& image)
{
    int num_rows = image.size(); //Gets the number of rows (i.e. height) in a 2D vector named image
    int num_columns = image[0].size(); //Gets the number of columns (i.e. width) in a 2D vector named image
    vector<vector<Pixel>> new_image(num_columns, vector<Pixel> (num_rows)); //define a new 2D vector with the Pixel values and set it to have the rows as number of columns and columns as the number or rows of the original image. 
    for (int row = 0; row < num_rows; row++)
    {
        for (int col = 0; col < num_columns; col++)
        {
            //Calculate the new row and new column when the image is rotated by 90 degrees
            int new_col = num_rows - row - 1;
            int new_row = col;
            //Obtain are the Pixel values for current cell & assign them to the corresponding cell located at new row and new column in the 90-degree rotated 2D vector
            new_image[new_row][new_col] = image[row][col];
        }
    }
    return new_image;
}

//PROCESS 5 - Rotate by multiples of 90 degrees
vector<vector<Pixel>> process_5(const vector<vector<Pixel>>& image, int number)
{
    int angle = number*90;
    vector<vector<Pixel>> new_image;
    //take the angle and divide it by 360 to get the remainder, rotate 0 time if remainder is 0
    if (angle%360 == 0) {new_image = image;}
    //take the angle and divide it by 360 to get the remainder, rotate 1 time if remainder is 90
    else if (angle%360 == 90) {new_image = process_4(image);}
    //take the angle and divide it by 360 to get the remainder, rotate 2 times if remainder is 180
    else if (angle%360 == 180) {new_image = process_4(process_4(image));}
    //take the angle and divide it by 360 to get the remainder, rotate 3 times if remainder is 270
    else {new_image = process_4(process_4(process_4(image)));}
    return new_image;
}

//PROCESS 6 - Enlarges in the x and y direction
vector<vector<Pixel>> process_6(const vector<vector<Pixel>>& image, int x_scale, int y_scale)
{
    int num_rows = image.size(); //Gets the number of rows (i.e. height) in a 2D vector named image
    int num_columns = image[0].size(); //Gets the number of columns (i.e. width) in a 2D vector named image
    int new_row = num_rows * y_scale; //Gets the new height
    int new_col = num_columns * x_scale; //Gets the new width
    vector<vector<Pixel>> new_image(new_row, vector<Pixel> (new_col)); //define a new 2D vector with the Pixel values and set it to have new size based on x_scale and y_scale
    for (int row = 0; row < new_row; row++)
    {
        for (int col = 0; col < new_col; col++)
        {
            //For each new pixel in new canvas, it takes on the corresponding pixel of the original image scaled back by x and y
            int factor_row = row/y_scale;
            int factor_col = col/x_scale;
            new_image[row][col] = image[factor_row][factor_col];
        }
    }
    return new_image;
}

//PROCESS 7 - Converts image to high contrast - black and white only
vector<vector<Pixel>> process_7(const vector<vector<Pixel>>& image) 
{
    int num_rows = image.size(); //Gets the number of rows (i.e. height) in a 2D vector named image
    int num_columns = image[0].size(); //Gets the number of columns (i.e. width) in a 2D vector named image
    vector<vector<Pixel>> new_image(num_rows, vector<Pixel> (num_columns)); //define a new 2D vector with the Pixel values and set it to have the same rows and columns as the original image. 
    for (int row = 0; row < num_rows; row++)
    {
        for (int col = 0; col < num_columns; col++)
        {
            //What are the red, blue and green values for current cell? Obtain the info and calculate the average (gray value)
            int red_value = image[row][col].red;
            int green_value = image[row][col].green;
            int blue_value = image[row][col].blue;
            double gray_value = (red_value + green_value + blue_value)/3;
            if (gray_value >= (255/2))
            {
            //If the cell is light, make it white & save the new color values to the corresponding pixel located at this row and column in the new 2D vector
                new_image[row][col].red = 255;
                new_image[row][col].green = 255;
                new_image[row][col].blue = 255;
            }
            //If the cell is dark, make it black & save the new color values to the corresponding pixel located at this row and column in the new 2D vector
            else
            {
                new_image[row][col].red = 0;
                new_image[row][col].green = 0;
                new_image[row][col].blue = 0;
            }
        }
    }
    return new_image;
}

//PROCESS 8 - Lightens image
vector<vector<Pixel>> process_8(const vector<vector<Pixel>>& image, double scaling_factor) 
{
    int num_rows = image.size(); //Gets the number of rows (i.e. height) in a 2D vector named image
    int num_columns = image[0].size(); //Gets the number of columns (i.e. width) in a 2D vector named image
    vector<vector<Pixel>> new_image(num_rows, vector<Pixel> (num_columns)); //define a new 2D vector with the Pixel values and set it to have the same rows and columns as the original image. 
    for (int row = 0; row < num_rows; row++)
    {
        for (int col = 0; col < num_columns; col++)
        {
            //What are the red, blue and green values for current cell?
            int red_value = image[row][col].red;
            int green_value = image[row][col].green;
            int blue_value = image[row][col].blue;
            //Calculate the new red, green and blue values based on scaling factor 
            int new_red = 255 - (255 - red_value)* scaling_factor;
            int new_green = 255 - (255 - green_value)* scaling_factor;
            int new_blue = 255 - (255 - blue_value)* scaling_factor;
            //and assign those to the corresponding pixel located at this row and column in the new 2D vector
            new_image[row][col].red = new_red;
            new_image[row][col].green = new_green;
            new_image[row][col].blue = new_blue;
        }
    }
    return new_image;
}

//PROCESS 9 - Darkens image
vector<vector<Pixel>> process_9(const vector<vector<Pixel>>& image, double scaling_factor) 
{
    int num_rows = image.size(); //Gets the number of rows (i.e. height) in a 2D vector named image
    int num_columns = image[0].size(); //Gets the number of columns (i.e. width) in a 2D vector named image
    vector<vector<Pixel>> new_image(num_rows, vector<Pixel> (num_columns)); //define a new 2D vector with the Pixel values and set it to have the same rows and columns as the original image. 
    for (int row = 0; row < num_rows; row++)
    {
        for (int col = 0; col < num_columns; col++)
        {
            //What are the red, blue and green values for current cell?
            int red_value = image[row][col].red;
            int green_value = image[row][col].green;
            int blue_value = image[row][col].blue;
            //Calculate the new red, green and blue values based on scaling factor - multiply current value by a scaling factor less than 1 to make the color darker
            int new_red = red_value * scaling_factor;
            int new_green = green_value * scaling_factor;
            int new_blue = blue_value * scaling_factor;
            //and assign those to the corresponding pixel located at this row and column in the new 2D vector
            new_image[row][col].red = new_red;
            new_image[row][col].green = new_green;
            new_image[row][col].blue = new_blue;
        }
    }
    return new_image;
}

//PROCESS 10 - Converts to only black, white, red, blue and green
vector<vector<Pixel>> process_10(const vector<vector<Pixel>>& image)
{
    int num_rows = image.size(); //Gets the number of rows (i.e. height) in a 2D vector named image
    int num_columns = image[0].size(); //Gets the number of columns (i.e. width) in a 2D vector named image
    vector<vector<Pixel>> new_image(num_rows, vector<Pixel> (num_columns)); //define a new 2D vector with the Pixel values and set it to have the same rows and columns as the original image. 
    for (int row = 0; row < num_rows; row++)
    {
        for (int col = 0; col < num_columns; col++)
        {
            //What are the red, blue and green values for current cell? Obtain the info and calculate the max value
            int red_value = image[row][col].red;
            int green_value = image[row][col].green;
            int blue_value = image[row][col].blue;
            //If the cell is light, make it white
            if (red_value + green_value + blue_value >= 550)
            {
                int new_red = 255;
                int new_green = 255;
                int new_blue = 255;
                //Save the new color values to the corresponding pixel located at this row and column in the new 2D vector
                new_image[row][col].red = new_red;
                new_image[row][col].green = new_green;
                new_image[row][col].blue = new_blue;
            }
            //If the cell is dark, make it black
            else if (red_value + green_value + blue_value < 150)
            {
                int new_red = 0;
                int new_green = 0;
                int new_blue = 0;
                //Save the new color values to the corresponding pixel located at this row and column in the new 2D vector
                new_image[row][col].red = new_red;
                new_image[row][col].green = new_green;
                new_image[row][col].blue = new_blue;
            }
            //If the red value is highest, set it to max of 255 and set the other two colours to 0
            else if (red_value > green_value && red_value > blue_value)
            {
                int new_red = 255;
                int new_green = 0;
                int new_blue = 0;
                //Save the new color values to the corresponding pixel located at this row and column in the new 2D vector
                new_image[row][col].red = new_red;
                new_image[row][col].green = new_green;
                new_image[row][col].blue = new_blue;
            }
            //If the green value is highest, set it to max of 255 and set the other two colours to 0
            else if (green_value > red_value && green_value > blue_value)
            {
                int new_red = 0;
                int new_green = 255;
                int new_blue = 0;
                //Save the new color values to the corresponding pixel located at this row and column in the new 2D vector
                new_image[row][col].red = new_red;
                new_image[row][col].green = new_green;
                new_image[row][col].blue = new_blue;
            }
            else
            {
                int new_red = 0;
                int new_green = 0;
                int new_blue = 255;
                //Save the new color values to the corresponding pixel located at this row and column in the new 2D vector
                new_image[row][col].red = new_red;
                new_image[row][col].green = new_green;
                new_image[row][col].blue = new_blue;
            }
        }
    }
    return new_image;
}
    
int main()
{
    string file_name;
    cout <<""<<endl;
    cout <<"CSPB 1300 Image Processing Application"<<endl;
    cout <<""<<endl;
    cout <<"Enter input BMP filename: ";
    cin>> file_name;
    int input;
    cout <<""<<endl;
    cout <<"IMAGE PROCESSING MENU"<<endl;
    cout <<"0) Change image (current: "<<file_name<<")"<<endl;
    cout <<"1) Vignette"<<endl;
    cout <<"2) Clarendon"<<endl;
    cout <<"3) Grayscale"<<endl;
    cout <<"4) Rotate 90 degrees"<<endl;
    cout <<"5) Rotate multiple 90 degrees"<<endl;
    cout <<"6) Enlarge"<<endl;
    cout <<"7) High contrast"<<endl;
    cout <<"8) Lighten"<<endl;
    cout <<"9) Darken"<<endl;
    cout <<"10) Black, white, red, green, blue"<<endl;
    cout <<""<<endl;
    cout <<"Enter menu selection (Q to quit): ";
    while (cin>>input) 
    {
        if (input == 0)
        {
            string change_to;
            cout <<"Enter input BMP filename: ";
            cin>> change_to;
            file_name = change_to;
            cout <<"Successfully changed input image!"<<endl; 
        }
        else if (input == 1)
        {
            //Checking that the input file is a bmp file
            if (file_name.substr(file_name.length()-4) != ".bmp") 
            {
                cout<<"Input file should end with .bmp, please use option 0 to update"<<endl;
                continue;
            }
            cout <<""<<endl;
            cout <<"Vignette selected"<<endl;
            cout <<"Enter output BMP filename: ";
            string output_name;
            cin >> output_name; 
            vector<vector<Pixel>> test_image = read_image(file_name);
            vector<vector<Pixel>> test_image_1 = process_1(test_image);
            bool success_1 = write_image(output_name, test_image_1);
            cout <<"Successfully applied vignette!"<<endl;
        }
        else if (input == 2)
        {
            if (file_name.substr(file_name.length()-4) != ".bmp") 
            {
                cout<<"Input file should end with .bmp, please use option 0 to update"<<endl;
                continue;
            }
            cout <<""<<endl;
            cout <<"Clarendon selected"<<endl;
            cout <<"Enter output BMP filename: ";
            string output_name;
            cin >> output_name;
            cout <<"Enter scaling factor: ";
            double scaling_factor;
            cin >> scaling_factor;
            vector<vector<Pixel>> test_image = read_image(file_name);
            vector<vector<Pixel>> test_image_2 = process_2(test_image,scaling_factor);
            bool success_2 = write_image(output_name, test_image_2);
            cout <<"Successfully applied clarendon!"<<endl;
        }
        else if (input == 3)
        {
            if (file_name.substr(file_name.length()-4) != ".bmp") 
            {
                cout<<"Input file should end with .bmp, please use option 0 to update"<<endl;
                continue;
            }
            cout <<""<<endl;
            cout <<"Grayscale selected"<<endl;
            cout <<"Enter output BMP filename: ";
            string output_name;
            cin >> output_name;
            vector<vector<Pixel>> test_image = read_image(file_name);
            vector<vector<Pixel>> test_image_3 = process_3(test_image);
            bool success_3 = write_image(output_name, test_image_3);
            cout <<"Successfully applied grayscale!"<<endl;       
        }
        else if (input == 4)
        {
            if (file_name.substr(file_name.length()-4) != ".bmp") 
            {
                cout<<"Input file should end with .bmp, please use option 0 to update"<<endl;
                continue;
            }
            cout <<""<<endl;
            cout <<"Rotate 90 degrees selected"<<endl;
            cout <<"Enter output BMP filename: ";
            string output_name;
            cin >> output_name;
            vector<vector<Pixel>> test_image = read_image(file_name);
            vector<vector<Pixel>> test_image_4 = process_4(test_image);
            bool success_4 = write_image(output_name, test_image_4);
            cout <<"Successfully applied 90 degree rotation!"<<endl;     
        }
        else if (input == 5)
        {
            if (file_name.substr(file_name.length()-4) != ".bmp") 
            {
                cout<<"Input file should end with .bmp, please use option 0 to update"<<endl;
                continue;
            }
            cout <<""<<endl;
            cout <<"Rotate multiple 90 degrees selected"<<endl;
            cout <<"Enter output BMP filename: ";
            string output_name;
            cin >> output_name;
            cout <<"Enter number of 90-degree rotations: ";
            int number_of_rotations;
            cin >> number_of_rotations;
            vector<vector<Pixel>> test_image = read_image(file_name);
            vector<vector<Pixel>> test_image_5 = process_5(test_image,number_of_rotations);
            bool success_5 = write_image(output_name, test_image_5);
            cout <<"Successfully applied multiple 90-degree rotations!"<<endl;     
        }
        else if (input == 6)
        {
            if (file_name.substr(file_name.length()-4) != ".bmp") 
            {
                cout<<"Input file should end with .bmp, please use option 0 to update"<<endl;
                continue;
            }
            cout <<""<<endl;
            cout <<"Enlarge selected"<<endl;
            cout <<"Enter output BMP filename: ";
            string output_name;
            cin >> output_name;
            cout <<"Enter number X scale: ";
            int X_value ;
            cin >> X_value;
            cout <<"Enter number Y scale: ";
            int Y_value ;
            cin >> Y_value;
            vector<vector<Pixel>> test_image = read_image(file_name);
            vector<vector<Pixel>> test_image_6 = process_6(test_image,X_value,Y_value);
            bool success_6 = write_image(output_name, test_image_6);
            cout <<"Successfully enlarged!"<<endl;     
        }
        else if (input == 7)
        {
            if (file_name.substr(file_name.length()-4) != ".bmp") 
            {
                cout<<"Input file should end with .bmp, please use option 0 to update"<<endl;
                continue;
            }
            cout <<""<<endl;
            cout <<"High contrast selected"<<endl;
            cout <<"Enter output BMP filename: ";
            string output_name;
            cin >> output_name;
            vector<vector<Pixel>> test_image = read_image(file_name);
            vector<vector<Pixel>> test_image_7 = process_7(test_image);
            bool success_7 = write_image(output_name, test_image_7);
            cout <<"Successfully applied high contrast!"<<endl;     
        }
        else if (input == 8)
        {
            if (file_name.substr(file_name.length()-4) != ".bmp") 
            {
                cout<<"Input file should end with .bmp, please use option 0 to update"<<endl;
                continue;
            }
            cout <<""<<endl;
            cout <<"Lighten selected"<<endl;
            cout <<"Enter output BMP filename: ";
            string output_name;
            cin >> output_name;
            cout <<"Enter scaling factor: ";
            double scaling_factor;
            cin >> scaling_factor;
            vector<vector<Pixel>> test_image = read_image(file_name);
            vector<vector<Pixel>> test_image_8 = process_8(test_image,scaling_factor);
            bool success_8 = write_image(output_name, test_image_8);
            cout <<"Successfully lightened!"<<endl;     
        }
        else if (input == 9)
        {
            if (file_name.substr(file_name.length()-4) != ".bmp") 
            {
                cout<<"Input file should end with .bmp, please use option 0 to update"<<endl;
                continue;
            }
            cout <<""<<endl;
            cout <<"Darken selected"<<endl;
            cout <<"Enter output BMP filename: ";
            string output_name;
            cin >> output_name;
            cout <<"Enter scaling factor: ";
            double scaling_factor;
            cin >> scaling_factor;
            vector<vector<Pixel>> test_image = read_image(file_name);
            vector<vector<Pixel>> test_image_9 = process_9(test_image,scaling_factor);
            bool success_9 = write_image(output_name, test_image_9);
            cout <<"Successfully darkened!"<<endl;     
        }
        else if (input == 10)
        {
            if (file_name.substr(file_name.length()-4) != ".bmp") 
            {
                cout<<"Input file should end with .bmp, please use option 0 to update"<<endl;
                continue;
            }
            cout <<""<<endl;
            cout <<"Black, white, red, green, blue selected"<<endl;
            cout <<"Enter output BMP filename: ";
            string output_name;
            cin >> output_name;
            vector<vector<Pixel>> test_image = read_image(file_name);
            vector<vector<Pixel>> test_image_10 = process_10(test_image);
            bool success_10 = write_image(output_name, test_image_10);
            cout <<"Successfully applied black, white, red, green, blue filter!"<<endl;     
        }
        else if (input != 0 && input!=1 && input!=2 && input!=3 && input!=4 && input!=5 && input!=6 && input!=7 && input!=8 && input!=9 && input!=10)
        {
            cout <<""<<endl;
            cout <<"WRONG INPUT ENTERED!!"<<endl;
            cout <<"Please enter a number between 0 and 10 or Q to quit";
            cout <<""<<endl;
        }
        cout <<""<<endl;
        cout <<"IMAGE PROCESSING MENU"<<endl;
        cout <<"0) Change image (current: "<<file_name<<")"<<endl;
        cout <<"1) Vignette"<<endl;
        cout <<"2) Clarendon"<<endl;
        cout <<"3) Grayscale"<<endl;
        cout <<"4) Rotate 90 degrees"<<endl;
        cout <<"5) Rotate multiple 90 degrees"<<endl;
        cout <<"6) Enlarge"<<endl;
        cout <<"7) High contrast"<<endl;
        cout <<"8) Lighten"<<endl;
        cout <<"9) Darken"<<endl;
        cout <<"10) Black, white, red, green, blue"<<endl;
        cout <<""<<endl;
        cout <<"Enter menu selection (Q to quit): ";
    }
    cout <<"Thank you for using my program!"<<endl;
    cout <<"Quitting... "<<endl;
    cout <<""<<endl;
    return 0;
}




            
            
    
    
    
