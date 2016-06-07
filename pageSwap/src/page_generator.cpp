#include <iostream>
#include <fstream>
#include <random>
#include <cstdint>
#include <exception>
#include <memory>
#include <functional>
#include <arpa/inet.h>


int main(int argc, char **argv) {

    try {

        std::cout.exceptions(std::ostream::failbit | std::ostream::badbit); // Throw if we break

        int seed = 1077; // The price of a cheese pizza and a large soda at Panucci's Pizza
        if (argc > 1) {
            seed = atoi(argv[1]);
        }

        std::clog << "Using seed " << seed << std::endl; //Hope it was a number, hah

        // Also, hope you're piping this to a file, haaaaaaaaaah

        std::mt19937 generator(seed);
        std::uniform_int_distribution<uint32_t> uni_distrib(0, 2047);
        std::normal_distribution<double> norm_distrib(1024, 10); // ~ 914-1054

        // "Fun" fact: Mersenne Twister implementation is defined, so it's consistent across platforms.
        // The distributions are not, so your numbers will still come out weird if you use them to
        //  transform the output. Yaaay...

        auto uniform_page = std::bind(uni_distrib, std::ref(generator));
        auto gaussian_page = std::bind(norm_distrib, std::ref(generator));


        // Generate a "fuckton" of requests, like Matt wanted
        // Let's make that, uhhh... 1M +- whatever the first uniform int generated because why not

        const uint32_t total = 1000000 + ((uniform_page() & 0x01) ? uniform_page() : -uniform_page());

        std::clog << "Generating " << total << " requests." << std::endl;

        std::unique_ptr<uint32_t[]> request_file(new uint32_t[total + 1]);

        request_file[0] = total;

        for (uint32_t idx = 1; idx < total; ++idx) {
            // gaussian every 8
            request_file[idx] = (idx & 0x07) ? uniform_page() : gaussian_page();
        }

        std::cout.write((const char *)request_file.get(), (total + 1) << 2); // Eh, whatever, if it breaks, it'll throw.


    } catch (std::ostream::failure e) {
        std::clog << "Cout broke because: " << e.what() << " How sad." << std::endl;
    } catch (std::bad_alloc e) {
        std::clog << "Allocation broke because: " << e.what() << " How sad." << std::endl;
    } catch (...) {
        std::clog << "Something mysterious happened." << std::endl;
    }


}
