///mafiagh.h///
// Platform independence header
// Will add to this as things become needed

/////////////////////////////////////////
//WILL BE MORPHED INTO A GENERAL HEADER//
/////////////////////////////////////////

#ifdef _MSC_VER
  #include <cstdlib>
  #define SLEEP(x) Sleep(x)
#elif defined __GNUC__
  #include <unistd.h>
  #define SLEEP(x) usleep(x)
#endif
