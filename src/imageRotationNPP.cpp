/* Copyright (c) 2019, NVIDIA CORPORATION. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *  * Neither the name of NVIDIA CORPORATION nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#if defined(WIN32) || defined(_WIN32) || defined(WIN64) || defined(_WIN64)
#define WINDOWS_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#pragma warning(disable : 4819)
#endif

#include <Exceptions.h>
#include <ImageIO.h>
#include <ImagesCPU.h>
#include <ImagesNPP.h>

#include <string.h>
#include <fstream>
#include <iostream>

#include <cuda_runtime.h>
#include <npp.h>

#include <helper_cuda.h>
#include <helper_string.h>

bool printfNPPinfo(int argc, char *argv[])
{
    const NppLibraryVersion *libVer = nppGetLibVersion();
    printf("NPP Library Version %d.%d.%d\n", libVer->major, libVer->minor, libVer->build);

    int driverVersion, runtimeVersion;
    cudaDriverGetVersion(&driverVersion);
    cudaRuntimeGetVersion(&runtimeVersion);

    printf("  CUDA Driver  Version: %d.%d\n", driverVersion / 1000, (driverVersion % 100) / 10);
    printf("  CUDA Runtime Version: %d.%d\n", runtimeVersion / 1000, (runtimeVersion % 100) / 10);

    return checkCudaCapabilities(1, 0);
}

int main(int argc, char *argv[])
{
    printf("%s Starting...\n\n", argv[0]);

    try
    {
        std::string sFilename;
        char *filePath;

        findCudaDevice(argc, (const char **)argv);

        if (!printfNPPinfo(argc, argv))
        {
            exit(EXIT_SUCCESS);
        }

        if (checkCmdLineFlag(argc, (const char **)argv, "input"))
        {
            getCmdLineArgumentString(argc, (const char **)argv, "input", &filePath);
        }
        else
        {
            filePath = sdkFindFilePath("Lena.pgm", argv[0]);
        }

        sFilename = filePath ? filePath : "Lena.pgm";

        std::ifstream infile(sFilename.data());
        if (!infile.good())
        {
            std::cerr << "Unable to open: <" << sFilename.data() << ">\n";
            exit(EXIT_FAILURE);
        }
        infile.close();

        std::string sResultFilename = sFilename;
        std::string::size_type dot = sResultFilename.rfind('.');
        if (dot != std::string::npos)
        {
            sResultFilename = sResultFilename.substr(0, dot);
        }
        sResultFilename += "_mirror.pgm";

        if (checkCmdLineFlag(argc, (const char **)argv, "output"))
        {
            char *outputFilePath;
            getCmdLineArgumentString(argc, (const char **)argv, "output", &outputFilePath);
            sResultFilename = outputFilePath;
        }

        // Host and Device image objects
        npp::ImageCPU_8u_C1 oHostSrc;
        npp::loadImage(sFilename, oHostSrc);

        npp::ImageNPP_8u_C1 oDeviceSrc(oHostSrc);
        npp::ImageNPP_8u_C1 oDeviceDst(oDeviceSrc.size());

        // Perform horizontal flip (mirror)
        NPP_CHECK_NPP(nppiMirror_8u_C1R(
            oDeviceSrc.data(), oDeviceSrc.pitch(),
            oDeviceDst.data(), oDeviceDst.pitch(),
            { (int)oDeviceSrc.width(), (int)oDeviceSrc.height() },
            NPP_HORIZONTAL));

        npp::ImageCPU_8u_C1 oHostDst(oDeviceDst.size());
        oDeviceDst.copyTo(oHostDst.data(), oHostDst.pitch());

        saveImage(sResultFilename, oHostDst);
        std::cout << "Saved mirror image to: " << sResultFilename << std::endl;

        nppiFree(oDeviceSrc.data());
        nppiFree(oDeviceDst.data());

        exit(EXIT_SUCCESS);
    }
    catch (npp::Exception &rException)
    {
        std::cerr << "NPP Exception:\n" << rException << std::endl;
        exit(EXIT_FAILURE);
    }
    catch (...)
    {
        std::cerr << "Unknown exception occurred.\n";
        exit(EXIT_FAILURE);
    }

    return 0;
}
