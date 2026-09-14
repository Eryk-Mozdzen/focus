#!/usr/bin/env -S uv run --script

# /// script
# requires-python = ">=3.10"
# dependencies = [
#     "pathlib",
#     "numpy",
#     "matplotlib",
# ]
# ///

import subprocess
import pathlib
import numpy as np
import matplotlib.pyplot as plt

BUILD_PATH = pathlib.Path("build")


def wrap(angle):
    return (angle + np.pi) % (2 * np.pi) - np.pi


class Function:
    def __init__(self, function, args):
        self.function = function
        self.args = args

    def build(self, directory):
        filepath = directory / pathlib.Path(self.function + ".c")
        self.executable = directory / pathlib.Path(self.function)
        filepath.parent.mkdir(parents=True, exist_ok=True)
        with open(filepath, "w") as file:
            if self.args == 1:
                file.write(f"""#include <stdio.h>
    #include <stdlib.h>

    #include "focus/math.h"

    int main(int argc, char **argv) {{
        const float arg = strtof(argv[1], NULL);
        const float result = {self.function}(arg);
        printf("%f\\n", result);
        return 0;
    }}
    """)
            if self.args == 2:
                file.write(f"""#include <stdio.h>
    #include <stdlib.h>

    #include "focus/math.h"

    int main(int argc, char **argv) {{
        const float arg1 = strtof(argv[1], NULL);
        const float arg2 = strtof(argv[2], NULL);
        const float result = {self.function}(arg1, arg2);
        printf("%f\\n", result);
        return 0;
    }}
    """)
        subprocess.run(
            [
                "gcc",
                "-I",
                "../focus/include",
                "-Os",
                "-c",
                "../focus/math.c",
                "-o",
                str(filepath.parent / "math.o"),
            ],
        )
        subprocess.run(
            [
                "gcc",
                "-I",
                "../focus/include",
                "-Os",
                "-c",
                "../focus/math_lookup_sin.c",
                "-o",
                str(filepath.parent / "math_lookup_sin.o"),
            ],
        )
        subprocess.run(
            [
                "gcc",
                "-I",
                "../focus/include",
                "-Os",
                "-c",
                str(filepath),
                "-o",
                str(filepath.parent / self.function) + ".o",
            ]
        )
        subprocess.run(
            [
                "gcc",
                "math.o",
                "math_lookup_sin.o",
                self.function + ".o",
                "-o",
                self.function,
            ],
            cwd=filepath.parent,
        )

    def run(self, args):
        result = subprocess.run(
            [self.executable] + [f"{x:.9f}" for x in args],
            capture_output=True,
            text=True,
            check=True,
        )
        lines = result.stdout.strip().splitlines()
        return float(lines[0])


def test1d(fun, ref, x_min, x_max):
    print(fun.function)

    fun.build(BUILD_PATH / pathlib.Path(fun.function))

    xs = np.linspace(x_min, x_max, 10000)
    ref_value = ref(xs)
    fun_value = np.empty(len(xs))
    for i, x in enumerate(xs):
        fun_value[i] = fun.run([float(x)])

    fig, axs = plt.subplots(nrows=3, ncols=1, sharex=True)
    axs[0].plot(xs, ref_value, label=str(ref))
    axs[0].plot(xs, fun_value, label=fun.function)
    axs[0].set_ylabel("value")
    axs[0].grid()
    axs[0].legend()

    axs[1].plot(xs, fun_value - ref_value)
    axs[1].set_ylabel("abs error")
    axs[1].grid()

    axs[2].plot(xs, (fun_value - ref_value) / ref_value)
    axs[2].set_xlabel("arg")
    axs[2].set_ylabel("rel error")
    axs[2].grid()


def test2d(fun, ref):
    print(fun.function)

    fun.build(BUILD_PATH / pathlib.Path(fun.function))

    xs = np.linspace(-100, 100, 100)
    ys = np.linspace(-100, 100, 100)
    ref_value = ref(*np.meshgrid(ys, xs, indexing="ij"))
    fun_value = np.empty((len(ys), len(xs)))
    for i, x in enumerate(xs):
        for j, y in enumerate(ys):
            fun_value[j, i] = fun.run([float(y), float(x)])

    fig, axs = plt.subplots(nrows=2, ncols=2)

    im = axs[0, 0].imshow(np.rad2deg(ref_value), aspect=1)
    fig.colorbar(im, ax=axs[0, 0], label=f"{ref} [deg]")

    im = axs[0, 1].imshow(np.rad2deg(fun_value), aspect=1)
    fig.colorbar(im, ax=axs[0, 1], label=f"{fun.function} [deg]")

    im = axs[1, 0].imshow(np.rad2deg(wrap(fun_value - ref_value)), aspect=1)
    fig.colorbar(im, ax=axs[1, 0], label="rel error [deg]")


def main():
    test1d(Function("focus_math_abs", 1), np.abs, -10, 10)
    test1d(Function("focus_math_sqrt", 1), np.sqrt, 0, 1000)
    test1d(Function("focus_math_exp", 1), np.exp, -100, 25)
    test1d(Function("focus_math_sin", 1), np.sin, -10, 10)
    test1d(Function("focus_math_cos", 1), np.cos, -10, 10)
    test2d(Function("focus_math_atan2", 2), np.atan2)
    test1d(Function("focus_math_angle_wrap", 1), wrap, -10, 10)

    plt.show()


if __name__ == "__main__":
    main()
