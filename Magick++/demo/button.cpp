//
// Magick++ demo to generate a simple text button
//
// Bob Friesenhahn, 1999, 2000
// 

#include <string>
#include <iostream>

#include <Magick++.h>

using namespace std;

using namespace Magick;

int main( int /*argc*/, char ** argv)
{

  // Initialize ImageMagick install location for Windows
  MagickIncarnate(*argv);

  string srcdir("");
  if(getenv("srcdir") != 0)
    srcdir = getenv("srcdir") + string("/");

  try {

    //
    // Options
    //

    string backGround = "xc:#CCCCCC"; // A solid color

    // Color to use for decorative border
    Color border = "#D4DCF3";

    // Button size
    string buttonSize = "120x20";

    // Button background texture
    string buttonTexture = "granite:";

    // Button text
    string text = "Button Text";

    // Button text color
    string textColor = "red";

    // Font to use for text
    string textFont = srcdir + "Generic.ttf";

    // Font point size
    int textFontPointSize = 16;

    //
    // Magick++ operations
    //

    Image button;

    // Set button size
    button.size( buttonSize );

    // Read background image
    button.read( backGround );

    // Set background to buttonTexture
    Image backgroundTexture( buttonTexture );
    button.texture( backgroundTexture );

    // Add some text
    button.penColor( textColor );
    button.fontPointsize( textFontPointSize );
    button.font( textFont );
    button.annotate( text, CenterGravity );

    // Add a decorative frame
    button.borderColor( border );
    button.frame( "6x6+3+3" );

    // Quantize to desired colors
    button.quantizeColors(16);
    button.quantize();

    // Save to file
    cout << "Writing to button.miff" << endl;
    button.write("button.miff");

    // Display on screen
    // button.display();

  }
  catch( exception error_ )
    {
      cout << "Caught exception: " << error_.what() << endl;
      return 1;
    }
  
  return 0;
}
