// progress_bar.h
//
// a simple progress bar for terminal output

#ifndef PROGRESS_BAR_H
#define PROGRESS_BAR_H

#include <unistd.h>
#include <stdio.h>

class progress_bar {
    FILE *output = nullptr;
    static constexpr const char *non = "............................................................";
    static constexpr const char *bar = "||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||";
    size_t pbwidth = strlen(bar);

public:

    progress_bar() : output{nullptr} { }

    progress_bar(FILE *f) {
        set_output(f);
        pbwidth = strlen(bar);    // note: could find terminal width from tty_ioctl(TIOCGWINSZ)
    }

    void set_output(FILE *f) {
        if (isatty(fileno(f))) {  // TODO: conditional compilation around POSIX functions
            output = f;
        }
    }

    void print_bar(size_t iteration, size_t num_trials) {
        if (output) {

            // avoid division by zero
            if (num_trials == 0) {
                return;
            }

            // we only need to update the progress bar at most
            // num_trials/100 times; for most iterations, we can just
            // return and avoid the extra computation needed to print
            // out the bar
            //
            if (iteration % (num_trials/100) != 0 && iteration != num_trials-1) {
                return;
            }

            // compute length of left and right parts of bar
            //
            double fraction = (double) (iteration+1) / num_trials;
            int percent = (int) (fraction * 100);
            int lpad = (int) (fraction * pbwidth);
            int rpad = pbwidth - lpad;

            // display progress bar
            //
            fprintf(output, "\033[;32m");
            fprintf(output, "\r%3d%% [%.*s%.*s]", percent, lpad, bar, rpad, non);
            fprintf(output, "\033[0m");
            fflush(output);

            // advance to next line, if we are finished
            //
            if (iteration == num_trials-1) {
                fputc('\n', output);
            }
        }
    }

};

#endif // PROGRESS_BAR_H
