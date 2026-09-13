#!/usr/bin/env -S uv run --script

# /// script
# requires-python = ">=3.10"
# dependencies = [
#     "matplotlib",
#     "numpy",
# ]
# ///

import subprocess
import sys

import matplotlib.pyplot as plt
import numpy as np


def run_custom_sincos(program, x):
    result = subprocess.run(
        [program, f"{x:.9f}"],
        capture_output=True,
        text=True,
        check=True,
    )

    lines = result.stdout.strip().splitlines()

    if len(lines) < 2:
        raise RuntimeError(
            f"Expected two output lines from {program}, got:\n" f"{result.stdout}"
        )

    sin_value = float(lines[0])
    cos_value = float(lines[1])

    return sin_value, cos_value


def main():
    program = sys.argv[1] if len(sys.argv) > 1 else "./a.out"

    xs = np.linspace(-10.0, 10.0, 4000)

    print(f"Running: {program}")
    print(f"Testing {len(xs)} points...")

    custom_sin = np.empty(len(xs))
    custom_cos = np.empty(len(xs))

    for i, x in enumerate(xs):
        custom_sin[i], custom_cos[i] = run_custom_sincos(program, float(x))

    real_sin = np.sin(xs)
    real_cos = np.cos(xs)

    sin_error = custom_sin - real_sin
    cos_error = custom_cos - real_cos

    sin_max_index = np.argmax(np.abs(sin_error))
    cos_max_index = np.argmax(np.abs(cos_error))

    print()
    print("SIN")
    print("----------------------------------------")
    print(f"Maximum absolute error: {abs(sin_error[sin_max_index]):.9e}")
    print(f"At x:                   {xs[sin_max_index]:.9f}")
    print(f"Custom:                 {custom_sin[sin_max_index]:.9e}")
    print(f"Reference:              {real_sin[sin_max_index]:.9e}")

    print()
    print("COS")
    print("----------------------------------------")
    print(f"Maximum absolute error: {abs(cos_error[cos_max_index]):.9e}")
    print(f"At x:                   {xs[cos_max_index]:.9f}")
    print(f"Custom:                 {custom_cos[cos_max_index]:.9e}")
    print(f"Reference:              {real_cos[cos_max_index]:.9e}")

    print()
    print("RMS ERROR")
    print("----------------------------------------")
    print(f"sin: {np.sqrt(np.mean(sin_error ** 2)):.9e}")
    print(f"cos: {np.sqrt(np.mean(cos_error ** 2)):.9e}")

    plt.figure()
    plt.plot(
        xs,
        real_sin,
        label="math.sin()",
        linewidth=2,
    )
    plt.plot(
        xs,
        custom_sin,
        label="focus_math_sin()",
        linewidth=1,
    )
    plt.plot(
        xs,
        real_cos,
        label="math.cos()",
        linewidth=2,
    )
    plt.plot(
        xs,
        custom_cos,
        label="focus_math_cos()",
        linewidth=1,
    )
    plt.xlabel("x [rad]")
    plt.ylabel("value")
    plt.title("Custom sin/cos vs reference")
    plt.grid()
    plt.legend()

    plt.figure()
    plt.plot(
        xs,
        sin_error,
        label="sin error",
    )
    plt.axhline(0.0, linewidth=0.8)
    plt.xlabel("x [rad]")
    plt.ylabel("error")
    plt.title("focus_math_sin() - math.sin()")
    plt.grid()
    plt.legend()

    plt.figure()
    plt.plot(
        xs,
        cos_error,
        label="cos error",
    )
    plt.axhline(0.0, linewidth=0.8)
    plt.xlabel("x [rad]")
    plt.ylabel("error")
    plt.title("focus_math_cos() - math.cos()")
    plt.grid()
    plt.legend()

    magnitude_error = custom_sin * custom_sin + custom_cos * custom_cos - 1.0

    print()
    print("SIN² + COS²")
    print("----------------------------------------")
    print(f"Maximum absolute error: " f"{np.max(np.abs(magnitude_error)):.9e}")
    print(f"RMS error: " f"{np.sqrt(np.mean(magnitude_error ** 2)):.9e}")

    plt.figure()
    plt.plot(
        xs,
        magnitude_error,
        label="sin²(x) + cos²(x) - 1",
    )
    plt.axhline(0.0, linewidth=0.8)
    plt.xlabel("x [rad]")
    plt.ylabel("error")
    plt.title("Unit-circle error of custom sin/cos")
    plt.grid()
    plt.legend()

    plt.show()


if __name__ == "__main__":
    main()
