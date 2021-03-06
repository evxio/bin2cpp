#include "Win32ResourceGenerator.h"
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
  Win32ResourceGenerator::Win32ResourceGenerator()
  {
  }

  Win32ResourceGenerator::~Win32ResourceGenerator()
  {
  }

  const char * Win32ResourceGenerator::getName() const
  {
    return "win32";
  }

  bool Win32ResourceGenerator::createCppSourceFile(const char * iCppFilePath)
  {
    bool resourceFileSuccess = createResourceFile(iCppFilePath);
    if (!resourceFileSuccess)
      return false;

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
    //long fileSize = getFileSize(input);
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
    fprintf(cpp, "#include <stdint.h>\n");
    fprintf(cpp, "\n");
    fprintf(cpp, "#ifndef WIN32_LEAN_AND_MEAN\n");
    fprintf(cpp, "#define WIN32_LEAN_AND_MEAN // Exclude rarely-used stuff from Windows headers\n");
    fprintf(cpp, "#endif\n");
    fprintf(cpp, "#include <windows.h>\n");
    fprintf(cpp, "\n");
    fprintf(cpp, "#include <psapi.h> //for EnumProcessModules()\n");
    fprintf(cpp, "#pragma comment( lib, \"psapi.lib\" )\n");
    fprintf(cpp, "\n");

    fprintf(cpp, "namespace %s\n", mNamespace.c_str());
    fprintf(cpp, "{\n");
    fprintf(cpp, "  class %s : public virtual %s::%s\n", className.c_str(), mNamespace.c_str(), mBaseClass.c_str());
    fprintf(cpp, "  {\n");
    fprintf(cpp, "  public:\n");
    fprintf(cpp, "    %s() :\n", className.c_str());
    fprintf(cpp, "      hProcess(NULL),\n");
    fprintf(cpp, "      hModule(NULL),\n");
    fprintf(cpp, "      hResourceInfoBlock(NULL),\n");
    fprintf(cpp, "      hResHandle(NULL),\n");
    fprintf(cpp, "      mBufferSize(0),\n");
    fprintf(cpp, "      mBuffer(NULL)\n");
    fprintf(cpp, "    {\n");
    fprintf(cpp, "      loadResource();\n");
    fprintf(cpp, "    }\n");
    fprintf(cpp, "    ~%s() { unloadResource(); }\n", className.c_str());
    fprintf(cpp, "    virtual size_t getSize() const { return mBufferSize; }\n");
    fprintf(cpp, "    virtual const char * getFilename() const { return \"%s\"; }\n", getFilename(mInputFile.c_str()).c_str());
    fprintf(cpp, "    virtual const char * getBuffer() const { return mBuffer; }\n");
    fprintf(cpp, "    void loadResource()\n");
    fprintf(cpp, "    {\n");
    fprintf(cpp, "      //Get a handle to this process\n");
    fprintf(cpp, "      hProcess = OpenProcess(  PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, GetCurrentProcessId() );\n");
    fprintf(cpp, "      if (hProcess)\n");
    fprintf(cpp, "      {\n");
    fprintf(cpp, "        //Find the main HMODULE of the process\n");
    fprintf(cpp, "        DWORD cbNeeded;\n");
    fprintf(cpp, "        if ( EnumProcessModules( hProcess, &hModule, sizeof(hModule), &cbNeeded) )\n");
    fprintf(cpp, "        {\n");
    fprintf(cpp, "          //Retrieve the resource\n");
    fprintf(cpp, "          hResourceInfoBlock = FindResource(hModule, \"%s\", \"CUSTOM\");\n", getRandomIdentifier(iCppFilePath).c_str());
    fprintf(cpp, "          if (hResourceInfoBlock)\n");
    fprintf(cpp, "          {\n");
    fprintf(cpp, "            hResHandle = LoadResource(hModule, hResourceInfoBlock);\n");
    fprintf(cpp, "            if (hResHandle)\n");
    fprintf(cpp, "            {\n");
    fprintf(cpp, "              mBuffer = (const char *)LockResource(hResHandle);\n");
    fprintf(cpp, "              mBufferSize = SizeofResource(hModule, hResourceInfoBlock);\n");
    fprintf(cpp, "            }\n");
    fprintf(cpp, "          }\n");
    fprintf(cpp, "        }\n");
    fprintf(cpp, "      }\n");
    fprintf(cpp, "    }\n");
    fprintf(cpp, "    virtual void unloadResource()\n");
    fprintf(cpp, "    {\n");
    fprintf(cpp, "      if (hResHandle)\n");
    fprintf(cpp, "      {\n");
    fprintf(cpp, "        FreeResource(hResHandle);\n");
    fprintf(cpp, "        hResHandle = NULL;\n");
    fprintf(cpp, "        mBuffer = NULL;\n");
    fprintf(cpp, "        mBufferSize = 0;\n");
    fprintf(cpp, "      }\n");
    fprintf(cpp, "      hResourceInfoBlock = NULL;\n");
    fprintf(cpp, "      hModule = NULL;\n");
    fprintf(cpp, "      if (hProcess)\n");
    fprintf(cpp, "      {\n");
    fprintf(cpp, "        CloseHandle(hProcess);\n");
    fprintf(cpp, "        hProcess = NULL;\n");
    fprintf(cpp, "      }\n");
    fprintf(cpp, "    }\n");
    fprintf(cpp, "%s", getSaveMethodTemplate().c_str());
    fprintf(cpp, "  private:\n");
    fprintf(cpp, "    HANDLE hProcess;\n");
    fprintf(cpp, "    HMODULE hModule;\n");
    fprintf(cpp, "    HRSRC hResourceInfoBlock;\n");
    fprintf(cpp, "    HGLOBAL hResHandle;\n");
    fprintf(cpp, "    DWORD mBufferSize;\n");
    fprintf(cpp, "    const char * mBuffer;\n");
    fprintf(cpp, "  };\n");
    fprintf(cpp, "  const %s & %s() { static %s _instance; return _instance; }\n", mBaseClass.c_str(), getterFunctionName.c_str(), className.c_str());
    fprintf(cpp, "}; //%s\n", mNamespace.c_str());

    fclose(input);
    fclose(cpp);

    return true;
  }

  std::string Win32ResourceGenerator::getResourceFilePath(const char * iCppFilePath)
  {
    //Build header file path
    std::string resourcePath = iCppFilePath;
    strReplace(resourcePath, ".cpp", ".rc");
    return resourcePath;
  }

  bool Win32ResourceGenerator::createResourceFile(const char * iCppFilePath)
  {
    //Build resource file path
    std::string resourceFilePath = getResourceFilePath(iCppFilePath);

    //create resource file
    FILE * res = fopen(resourceFilePath.c_str(), "w");
    if (!res)
    {
      return false;
    }

    std::string filePath = mInputFile;
    strReplace(filePath, "\\", "\\\\");

    //write res file heading
    fprintf(res, "%s", getHeaderTemplate().c_str());
    fprintf(res, "#include <windows.h>\n");
    fprintf(res, "%s CUSTOM \"%s\"\n", getRandomIdentifier(iCppFilePath).c_str(), filePath.c_str());

    fclose(res);

    return true;
  }

  std::string Win32ResourceGenerator::getRandomIdentifier(const char * /*iCppFilePath*/)
  {
    return "html5skeletonAGE632H2D7";
  }

}; //bin2cpp