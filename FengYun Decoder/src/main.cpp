#ifndef _WIN32
#include <unistd.h>
#else
#include "getopt/getopt.h"
#endif

#include <iostream>
#include <fstream>
#include <complex>
#include <vector>
#include <thread>

#include "viterbi.h"
#include "diff.h"

// #include "tclap/CmdLine.h"
#include <tclap/CmdLine.h>

// Processing buffer size
#define BUFFER_SIZE (8192 * 5)

// Small function that returns 1 bit from any type
template <typename T>
inline bool getBit(T data, int bit)
{
    return (data >> bit) & 1;
}

// Return filesize
size_t getFilesize(std::string filepath)
{
    std::ifstream file(filepath, std::ios::binary | std::ios::ate);
    std::size_t fileSize = file.tellg();
    file.close();
    return fileSize;
}

int main(int argc, char *argv[])
{

    TCLAP::CmdLine cmd("FengYun Decoder by Aang23", ' ', "1.1");

    // File arguments
    TCLAP::ValueArg<std::string> valueInput("i", "input", "Raw input", true, "", "symbols.bin");
    TCLAP::ValueArg<std::string> valueOutput("o", "output", "Output frames", true, "", "outputframes.bin");

    // Arguments to extract
    TCLAP::SwitchArg valueFYab("b", "fyab", "Decode FengYun A / B satellite (default)", true);
    TCLAP::SwitchArg valueFYcd("c", "fycd", "Decode FengYun C", false);
    TCLAP::ValueArg<float> valueVit("v", "viterbi", "Viterbi threshold (default: 0.170)", false, 0.170, "threshold");
    TCLAP::ValueArg<int> valueOutsync("s", "outsync", "Outsync after no. frames (default: 5)", false, 5, "frames");
    TCLAP::SwitchArg valueVerbose("V", "verbose", "Show more output", false);

    // Register all of the above options
    cmd.add(valueFYab);
    cmd.add(valueFYcd);
    cmd.add(valueVit);
    cmd.add(valueOutsync);
    cmd.add(valueInput);
    cmd.add(valueOutput);
    cmd.add(valueVerbose);
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
    float viterbi_ber_threasold = valueVit.getValue();
    int fy3c_mode;

    if (valueFYab.getValue())
    {
        int fy3c_mode = 0;
    }
    else if (valueFYcd.getValue())
    {
        int fy3c_mode = 1;
    }
    int sw = 0;

    // Output and Input file
    std::ifstream data_in(valueInput.getValue(), std::ios::binary);
    std::ofstream data_out(valueOutput.getValue(), std::ios::binary);

    // Our 2 Viterbi decoders and differential decoder
    FengyunViterbi viterbi1(true, viterbi_ber_threasold, 1, viterbi_outsync_after, 50), viterbi2(true, viterbi_ber_threasold, 1, viterbi_outsync_after, 50);
    FengyunDiff diff;

    // Viterbi output buffer
    uint8_t *viterbi1_out = new uint8_t[BUFFER_SIZE];
    uint8_t *viterbi2_out = new uint8_t[BUFFER_SIZE];

    // A few vectors for processing
    std::vector<std::complex<float>> *iSamples = new std::vector<std::complex<float>>(BUFFER_SIZE),
                                     *qSamples = new std::vector<std::complex<float>>(BUFFER_SIZE);

    // Read buffer
    std::complex<float> buffer[BUFFER_SIZE];

    // Diff decoder input and output
    std::vector<uint8_t> *diff_in = new std::vector<uint8_t>, *diff_out = new std::vector<uint8_t>;

    // Complete filesize
    size_t filesize = getFilesize(valueInput.getValue());

    // Data we wrote out
    size_t data_out_total = 0;

    std::cout << "---------------------------" << std::endl;
    std::cout << "FengYun Decoder by Aang23" << std::endl;
    std::cout << "Fixed by Tomi HA6NAB" << std::endl;
    std::cout << "---------------------------" << std::endl;
    std::cout << "Viterbi threshold: " << viterbi_ber_threasold << std::endl;
    std::cout << "Outsinc after: " << viterbi_outsync_after << std::endl;
    std::cout << "---------------------------" << std::endl;
    std::cout << std::endl;

    int shift = 0;
    if (!valueVerbose.getValue())
    {
        std::cout << "Running" << std::endl;
    }

    // Read until there is no more data
    while (!data_in.eof())
    {
        // Read a buffer
        data_in.read((char *)buffer, sizeof(std::complex<float>) * BUFFER_SIZE);

        // Deinterleave I & Q for the 2 Viterbis
        for (int i = 0; i < BUFFER_SIZE / 2; i++)
        {
            using namespace std::complex_literals;
            std::complex<float> iS = buffer[i * 2 + shift].imag() + buffer[i * 2 + 1 + shift].imag() * 1if;
            std::complex<float> qS = buffer[i * 2 + shift].real() + buffer[i * 2 + 1 + shift].real() * 1if;
            iSamples->push_back(iS);
            if (fy3c_mode)
            {
                qSamples->push_back(-qS); //FY3C
            }
            else
            {
                qSamples->push_back(qS); // FY3B
            }
        }
        // Run Viterbi!
        int v1 = viterbi1.work(*qSamples, qSamples->size(), viterbi1_out);
        int v2 = viterbi2.work(*iSamples, iSamples->size(), viterbi2_out);

        // Interleave and pack output into 2 bits chunks
        if (v1 > 0 || v2 > 0)
        {
            if (v1 == v2 && v1 > 0)
            {
                uint8_t bit1, bit2, bitCb;
                for (int y = 0; y < v1; y++)
                {
                    for (int i = 7; i >= 0; i--)
                    {
                        bit1 = getBit<uint8_t>(viterbi1_out[y], i);
                        bit2 = getBit<uint8_t>(viterbi2_out[y], i);
                        bitCb = bit2 << 1 | bit1;
                        diff_in->push_back(bitCb);
                    }
                }
            }
        }
        else
        {
            if (shift)
            {
                shift = 0;
            }
            else
            {
                shift = 1;
            }
            diff_in->clear();
            iSamples->clear();
            qSamples->clear();
            // Deinterleave I & Q for the 2 Viterbis
            for (int i = 0; i < BUFFER_SIZE / 2; i++)
            {
                using namespace std::complex_literals;
                std::complex<float> iS = buffer[i * 2 + shift].imag() + buffer[i * 2 + 1 + shift].imag() * 1if;
                std::complex<float> qS = buffer[i * 2 + shift].real() + buffer[i * 2 + 1 + shift].real() * 1if;
                iSamples->push_back(iS);
                if (fy3c_mode)
                {
                    qSamples->push_back(-qS); //FY3C
                }
                else
                {
                    qSamples->push_back(qS); // FY3B
                }
            }
            // Run Viterbi!
            int v1 = viterbi1.work(*qSamples, qSamples->size(), viterbi1_out);
            int v2 = viterbi2.work(*iSamples, iSamples->size(), viterbi2_out);

            // Interleave and pack output into 2 bits chunks
            if (v1 > 0 || v2 > 0)
            {
                if (v1 == v2 && v1 > 0)
                {
                    uint8_t bit1, bit2, bitCb;
                    for (int y = 0; y < v1; y++)
                    {
                        for (int i = 7; i >= 0; i--)
                        {
                            bit1 = getBit<uint8_t>(viterbi1_out[y], i);
                            bit2 = getBit<uint8_t>(viterbi2_out[y], i);
                            bitCb = bit2 << 1 | bit1;
                            diff_in->push_back(bitCb);
                        }
                    }
                }
            }
            else
            {
                if (shift)
                {
                    shift = 0;
                }
                else
                {
                    shift = 1;
                }
            }
        }

        // Perform differential decoding
        *diff_out = diff.work(*diff_in);

        // Reconstruct into bytes and write to output file
        for (int i = 0; i < diff_out->size() / 4; i++)
        {
            uint8_t toPush = ((*diff_out)[i * 4] << 6) | ((*diff_out)[i * 4 + 1] << 4) | ((*diff_out)[i * 4 + 2] << 2) | (*diff_out)[i * 4 + 3];
            data_out.write((char *)&toPush, 1);
        }

        data_out_total += diff_out->size() / 4;

        // Console stuff
        if (valueVerbose.getValue())
        {
            std::cout << '\r' << "Viterbi 1 : " << (viterbi1.getState() == 0 ? "NO SYNC" : viterbi1.getState() == 1 ? "SYNCING" : "SYNCED") << ", Viterbi 2 : " << (viterbi2.getState() == 0 ? "NO SYNC" : viterbi2.getState() == 1 ? "SYNCING" : "SYNCED") << ", Data out : " << round(data_out_total / 1e5) / 10.0f << " MB, Progress : " << round(((float)data_in.tellg() / (float)filesize) * 1000.0f) / 10.0f << "%     " << std::flush;
        }
        // Clear everything for the next run
        diff_in->clear();
        iSamples->clear();
        qSamples->clear();
    }

    std::cout << std::endl
              << "Done! Enjoy" << std::endl;

    // Close files
    data_in.close();
    data_out.close();
}
