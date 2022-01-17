// keystream_distribution.cc
//
// computes an empirical keystream distribution for the RC4 (or other)
// keystream generator(s)

// compilation:
//
//    g++ -Wall keystream_distribution.cc -O3 -std=c++17 -o keystream_distribution -ltbb
//
// libtbb is required; on debian/ubuntu, 'apt install libtbb-dev'


#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>
#include <array>
#include <random>
#include <stdexcept>

#include <thread>     // std::thread::hardware_concurrency()
#include <execution>  // std::for_each()
#include <numeric>    // std::accumulate()

#include "rc4.h"
#include "progress_bar.h"


// random source
//
std::random_device rd;
std::minstd_rand random_source(rd());

uint8_t random_uin8_t(void) {
    return random_source();
}

template <size_t L>
class random_uint8_array {
    std::array<uint8_t, L> a;

public:

    random_uint8_array() {
        for (auto &x : a) {
            x = random_source();
        }
    }

    const uint8_t *data() const { return a.data(); }
    const size_t size() const { return a.size(); }
};


template <typename keystream_generator>
class keystream_distribution {
    progress_bar progress;
    uint64_t count[256][256];

    void increment(size_t i, size_t j) {
        count[i][j]++;
    }

public:

    keystream_distribution() {
        bzero(count, sizeof(count)); // initialize to zero
    }

    keystream_distribution(const char *filename) {
        read_from_file(filename);
    }

    keystream_distribution(const keystream_distribution &rhs) {
        for (size_t i=0; i<256; i++) {
            for (size_t j=0; j<256; j++) {
                count[i][j] = rhs.count[i][j];
            }
        }
    }

    keystream_distribution & operator+(const keystream_distribution &rhs) {
        for (size_t i=0; i<256; i++) {
            for (size_t j=0; j<256; j++) {
                count[i][j] += rhs.count[i][j];
            }
        }
        return *this;
    }

    void fprint(FILE *f) {
        for (size_t i=0; i < 256; i++) {
            for (size_t j=0; j < 256; j++) {
                fprintf(f, "cnt[%zu][%zu]\t%lu\n", i, j, count[i][j]);
            }
        }
    }

    void write_to_file(const char *filename) {
        FILE *f = fopen(filename, "w");
        if (f == nullptr) {
            std::runtime_error("write_to_file() could not open file");
        }
        fprint(f);
        fclose(f);
    }

    void read_from_file(const char *filename) {
        FILE *f = fopen(filename, "r");
        if (f == nullptr) {
            std::runtime_error("read_from_file() could not open file");
        }

        // read in lines, parse format
        char *line = NULL;
        size_t len = 0;
        ssize_t nread;
        while ((nread = getline(&line, &len, f)) != -1) {
            size_t a, b, c;
            if (sscanf(line, "cnt[%zu][%zu]\t%zu\n", &a, &b, &c) == 3) {
                count[a][b] = c;
            } else {
                throw std::runtime_error("read_from_file(): error reading file");
            }
        }
        free(line);
        fclose(f);
    }

    template <size_t key_len=16>
    void compute(size_t num_trials) {

        for (size_t t = 0; t < num_trials; t++) {

            // generate random key and corresponding keystream
            //
            random_uint8_array<key_len> key;
            keystream_generator kg{key.data(), key.size()};
            std::array<uint8_t, 256> keystream;
            kg.write_keystream(keystream.data(), keystream.size());

            // update counters
            //
            for (size_t i = 0; i < sizeof(keystream); i++) {
                keystream_distribution::increment(i, keystream[i]);
            }

            // print progress bar (if configured to do so)
            //
            progress.print_bar(t, num_trials);
        }

    }

    void set_progress_bar_output(FILE *f) { progress.set_output(f); }

};


void usage(const char *progname) {
    fprintf(stderr,
            "Usage: %s [COMMAND], where COMMAND is one of:\n\n"
            "Compute command:\n\n"
            "   [trials=<num>] [input=<file>] [output=<file>] [concurrency=<num>] [verbose]\n\n"
            "   performs trial computations and creates/updates distribution, where\n"
            "      trials=<num> performs <num> trials\n"
            "      input=<file> uses distribution in <file> as initial distribution\n"
            "      output=<file> writes final distribution to <file>\n"
            "      concurrency=<num> uses <num> threads of execution\n\n"
            "Merge command:\n\n"
            "   merge <file1> <file2> [<file3> ... ] [output=<outfile>] [verbose]\n\n"
            "   reads distributions from two or more files, writes merged distribution\n\n"
            "Help command:\n\n"
            "   help\n\n"
            "   prints out this usage guidance\n\n"
            "NOTES\n\n"
            "   <num> can be an integer (e.g. 1024) or power of 2 (e.g. 2^10)\n\n"
            "   if output=<file> is not specified, the standard output is used\n\n"
            "   if conncurency=<num> is not specified, the number of cores is used\n\n"
            "   verbose keyword sends verbose output to standard error\n\n"
            "FILE FORMAT\n\n"
            "   cnt[i][j] counts the number of times the i^th byte of keystream equals j\n\n",
            progname);
}

int main(int argc, char *argv[]) {

    // parameters that are set by command line arguments, or otherwise
    // have default values, to control program behavior
    //
    size_t num_trials = 0;
    size_t concurrency = 0;
    char infile[64] = { '\0' };
    char outfile[64] = { '\0' };
    bool verbose = false;

    if (argc < 2) {
        usage(argv[0]);
        return EXIT_FAILURE;
    }

    // merge command
    //
    if (strcmp(argv[1], "merge") == 0) {
        std::vector<std::string> files_to_merge;
        for (int i=2; i<argc; i++) {

            // verbose output
            //
            if (strcmp(argv[i], "verbose") == 0) {
                verbose = true;
            }
            // outfile
            //
            else if (sscanf(argv[i], "output=%63s", outfile) == 1) {
                ;
            } else {
                files_to_merge.push_back(argv[i]);
            }
        }

        if (files_to_merge.size() < 2) {
            fprintf(stderr, "error: fewer than two files in merge operation\n");
            usage(argv[0]);
            return EXIT_FAILURE;
        }

        if (verbose) {
            for (const auto & f : files_to_merge) {
                fprintf(stderr, "%s ", f.c_str());
            }
            fputc('\n', stderr);
        }

        keystream_distribution<rc4> dist;
        for (const auto & f : files_to_merge) {
            fprintf(stderr, "merging in file %s\n", f.c_str());
            keystream_distribution<rc4> tmp_dist(f.c_str());
            dist = dist + tmp_dist;
        }
        if (outfile[0] != '\0') {
            dist.write_to_file(outfile);
        } else {
            dist.fprint(stdout);
        }

        return EXIT_SUCCESS;
    }

    for (int i=1; i<argc; i++) {
        size_t tmp = 0;

        // trials
        //
        if (sscanf(argv[i], "trials=2^%zu", &tmp) == 1) {
            num_trials = (size_t)1 << tmp;
        } else if (sscanf(argv[i], "trials=%zu", &tmp) == 1) {
            num_trials = tmp;
        }

        // concurrency
        //
        else if (sscanf(argv[i], "concurrency=%zu", &tmp) == 1) {
            concurrency = tmp;
        }

        // infile
        //
        else if (sscanf(argv[i], "input=%63s", infile) == 1) {
            ;
        }

        // outfile
        //
        else if (sscanf(argv[i], "output=%63s", outfile) == 1) {
            ;
        }

        // verbose output
        //
        else if (strcmp(argv[i], "verbose") == 0) {
            verbose = true;
        }

        // help
        //
        else if (strcmp(argv[i], "help") == 0) {
            usage(argv[0]);
            return EXIT_SUCCESS;
        }

        else {
            fprintf(stderr, "error: unrecognized term '%s'\n\n", argv[i]);
            usage(argv[0]);
            return EXIT_FAILURE;
        }
    }

    // if concurrency is unspecified, default to the hardware value
    //
    if (concurrency == 0) {
        concurrency = std::thread::hardware_concurrency();
        fprintf(stderr, "setting concurrency to number of cores (%zu)\n", concurrency);
    }

    // determine the number of trials per executor, rounding up if need be
    //
    size_t trials_per_exec = num_trials / concurrency;
    size_t remainder = num_trials - trials_per_exec * concurrency;
    if (remainder != 0) {
        trials_per_exec++;
        if (verbose) {
            fprintf(stderr, "performing %zu additional trials (num. trials not multiple of concurrency)\n", trials_per_exec * concurrency - num_trials);
        }
        num_trials = trials_per_exec * concurrency;
    }

    if (verbose) {
        fprintf(stderr, "num_trials: %zu\n", num_trials);
        fprintf(stderr, "concurrency: %zu\n", concurrency);
        fprintf(stderr, "trials_per_exec: %zu\n", trials_per_exec);

        fprintf(stderr, "infile: %s\n", infile[0] ? infile : "[none]");
        fprintf(stderr, "outfile: %s\n", outfile[0] ? outfile : "[none]");
    }

    // test keystream generator before use
    //
    if (rc4::test() == false) {
        fprintf(stderr, "error: rc4 failed self-test\n");
        return EXIT_FAILURE;
    }

    keystream_distribution<rc4> initial{};
    if (infile[0] != '\0') {
        fprintf(stderr, "reading initial distribution from file %s\n", infile);
        initial.read_from_file(infile);
    }

    std::vector<keystream_distribution<rc4>> dist(concurrency);
    dist[0].set_progress_bar_output(stderr);
    std::for_each(std::execution::par,
                  std::begin(dist), std::end(dist),
                  [num_trials](keystream_distribution<rc4> &d) { d.compute<16>(num_trials); });

    keystream_distribution<rc4> rc4_distribution = std::accumulate(std::begin(dist), std::end(dist), initial);

    if (outfile[0] != '\0') {
        rc4_distribution.write_to_file(outfile);
    } else {
        rc4_distribution.fprint(stdout);
    }

    return EXIT_SUCCESS;
}
