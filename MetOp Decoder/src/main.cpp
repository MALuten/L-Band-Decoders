#include <iostream>
#include <fstream>
#include <complex>
#include <vector>
// #include "tclap/CmdLine.h"
#include <tclap/CmdLine.h>

#include "viterbi.h"

#ifndef _WIN32
#include <unistd.h>
#else
#include "getopt/getopt.h"
#endif
// Return filesize
size_t getFilesize(std::string filepath)
{
    std::ifstream file(filepath, std::ios::binary | std::ios::ate);
    std::size_t fileSize = file.tellg();
    file.close();
    return fileSize;
}

// Processing buffer size
#define BUFFER_SIZE (8738 * 5)

int main(int argc, char *argv[])
{
    TCLAP::CmdLine cmd("MetOp Decoder by Aang23", ' ', "1.1");

    // File arguments
    TCLAP::ValueArg<std::string> valueInput("i", "input", "Symbols.", true, "", "symbols.bin");
    TCLAP::ValueArg<std::string> valueOutput("o", "output", "Output frames.", true, "", "outputframes.bin");

    // Arguments to extract
    TCLAP::ValueArg<float> valueVit("v", "viterbi", "Viterbi threshold (default: 0.170)", false, 0.170, "threshold");
    TCLAP::ValueArg<int> valueOutsync("s", "outsync", "Outsync after no. frames (default: 5)", false, 5, "frames");

    // Register all of the above options
    cmd.add(valueVit);
    cmd.add(valueOutsync);
    cmd.add(valueInput);
    cmd.add(valueOutput);
    // Parse
    try
    {
        cmd.parse(argc, argv);
    }
    catch (TCLAP::ArgException &e)
    {
        std::cout << e.error() << '\n';
        return 0;
    }
    // Variables
    int viterbi_outsync_after = valueOutsync.getValue();
    float viterbi_ber_threshold = valueVit.getValue();
    int sw = 0;

    // Output and Input file
    std::ifstream data_in(valueInput.getValue(), std::ios::binary);
    std::ofstream data_out(valueOutput.getValue(), std::ios::binary);

    // MetOp Viterbi decoder
    MetopViterbi viterbi(true, viterbi_ber_threshold, 1, viterbi_outsync_after, 50);

    // Viterbi output buffer
    uint8_t *viterbi_out = new uint8_t[BUFFER_SIZE];

    // Read buffer
    std::complex<float> buffer[BUFFER_SIZE];

    // Complete filesize
    size_t filesize = getFilesize(valueInput.getValue());

    // Data we wrote out
    size_t data_out_total = 0;

    // Print infos and credits out
    std::cout << "---------------------------" << std::endl;
    std::cout << "MetOp Decoder by Aang23" << std::endl;
    std::cout << "Fixed by Tomi HA6NAB" << std::endl;
    std::cout << "---------------------------" << std::endl;
    std::cout << "Viterbi threshold: " << viterbi_ber_threshold << std::endl;
    std::cout << "Outsinc after: " << viterbi_outsync_after << std::endl;
    std::cout << "---------------------------" << std::endl;
    std::cout << std::endl;

    // Read until EOF
    while (!data_in.eof())
    {
        // Read buffer
        data_in.read((char *)buffer, sizeof(std::complex<float>) * BUFFER_SIZE);

        // Push into Viterbi
        int num_samp = viterbi.work(buffer, BUFFER_SIZE, viterbi_out);

        data_out_total += num_samp;

        // Write output
        if (num_samp > 0)
            data_out.write((char *)viterbi_out, num_samp);

        // Console stuff
        std::cout << '\r' << "Viterbi : " << (viterbi.getState() == 0 ? "NO SYNC" : viterbi.getState() == 1 ? "SYNCING" : "SYNCED") << ", Data out : " << round(data_out_total / 1e5) / 10.0f << " MB, Progress : " << round(((float)data_in.tellg() / (float)filesize) * 1000.0f) / 10.0f << "%     " << std::flush;
    }

    std::cout << std::endl
              << "Done! Enjoy" << std::endl;

    data_in.close();
    data_out.close();
}
