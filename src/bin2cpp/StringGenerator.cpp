#include "StringGenerator.h"
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <stdlib.h>

#include "common.h"
#include "cppencoder.h"
#include "stringfunc.h"
#include "filesystemfunc.h"

using namespace stringfunc;
using namespace filesystem;

namespace bin2cpp
{
  StringGenerator::StringGenerator()
  {
  }

  StringGenerator::~StringGenerator()
  {
  }

  const char * StringGenerator::getName() const
  {
    return "string";
  }

  bool StringGenerator::createCppSourceFile(const char * iCppFilePath)
  {
    //check if input file exists
    FILE * input = fopen(mInputFile.c_str(), "rb");
    if (!input)
      return false;

    //Uppercase function identifier
    std::string functionIdentifier = capitalizeFirstCharacter(mFunctionIdentifier);

    //Build header and cpp file path
    std::string headerPath = getHeaderFilePath(iCppFilePath);
    std::string cppPath = iCppFilePath;
    std::string headerFilename = getFilename(headerPath.c_str());
    std::string cppFilename = getFilename(iCppFilePath);

    //create cpp file
    FILE * cpp = fopen(cppPath.c_str(), "w");
    if (!cpp)
    {
      fclose(input);
      return false;
    }

    //determine file properties
    long fileSize = getFileSize(input);
    std::string filename = getFilename(mInputFile.c_str());

    //Build class name
    std::string className;
    className.append(functionIdentifier.c_str());
    className.append("File");

    //Build function 
    std::string getterFunctionName = getGetterFunctionName();

    //write cpp file heading
    fprintf(cpp, "%s", getHeaderTemplate().c_str());
    fprintf(cpp, "#include \"%s\"\n", headerFilename.c_str() );
    fprintf(cpp, "#include <stdio.h> //for FILE\n");
    fprintf(cpp, "#include <string> //for memcpy\n");
    fprintf(cpp, "namespace %s\n", mNamespace.c_str());
    fprintf(cpp, "{\n");
    fprintf(cpp, "  class %s : public virtual %s::%s\n", className.c_str(), mNamespace.c_str(), mBaseClass.c_str());
    fprintf(cpp, "  {\n");
    fprintf(cpp, "  public:\n");
    fprintf(cpp, "    %s() {}\n", className.c_str());
    fprintf(cpp, "    ~%s() {}\n", className.c_str());
    fprintf(cpp, "    virtual size_t getSize() const { return %d; }\n", fileSize);
    fprintf(cpp, "    virtual const char * getFilename() const { return \"%s\"; }\n", getFilename(mInputFile.c_str()).c_str());
    fprintf(cpp, "    virtual const char * getBuffer() const\n");
    fprintf(cpp, "    {\n");
    fprintf(cpp, "      const char * buffer = ""\n");

    //create buffer for each chunks from input buffer
    int numLinePrinted = 0;
    unsigned char * buffer = new unsigned char[mChunkSize];
    while(!feof(input))
    {
      //read a chunk of the file
      size_t readSize = fread(buffer, 1, mChunkSize, input);

      bool isLastChunk = !(readSize == mChunkSize);

      if (readSize > 0)
      {
        if (numLinePrinted > 0)
        {
          //end previous line
          fprintf(cpp, "\n");
        }

        //convert to cpp string
        std::string cppEncoder;
        switch(mCppEncoder)
        {
        case IGenerator::CPP_ENCODER_HEX:
          cppEncoder = cppencoder::toHexString(buffer, readSize);
          break;
        case IGenerator::CPP_ENCODER_OCT:
        default:
          cppEncoder = cppencoder::toOctString(buffer, readSize, false);
          break;
        };

        //output
        fprintf(cpp, "        \"%s\"", cppEncoder.c_str());
        numLinePrinted++;
      }

      //end the string if last chunk printed
      if (isLastChunk)
      {
        fprintf(cpp, ";\n");
      }
    }
    delete[] buffer;
    buffer = NULL;

    //write cpp file footer
    fprintf(cpp, "      return buffer;\n");
    fprintf(cpp, "    }\n");
    fprintf(cpp, "%s", getSaveMethodTemplate().c_str());
    fprintf(cpp, "  };\n");
    fprintf(cpp, "  const %s & %s() { static %s _instance; return _instance; }\n", mBaseClass.c_str(), getterFunctionName.c_str(), className.c_str());
    fprintf(cpp, "}; //%s\n", mNamespace.c_str());

    fclose(input);
    fclose(cpp);

    return true;
  }

}; //bin2cpp