import subprocess
import os
import os.path as osp
from timeit import default_timer as timer
from argparse import ArgumentParser

parser = ArgumentParser()
traces_folder = "tests/traces"
cwd = os.getcwd()

POINTS = 60

all_traces = []
for file in os.listdir(traces_folder):
    if file.endswith(".sh"):
        all_traces.append(file[:-3])
all_traces = sorted(all_traces)

parser.add_argument("-t", "--trace", type=str, help="Specify the trace number")
parser.add_argument(
    "-V", "--verbose", action="store_true", help="Enable verbose output"
)
parser.add_argument("--anwser", "--ans", action="store_true", help="anwser traces")

args = parser.parse_args()

traces = args.trace.split(",") if args.trace else all_traces
for t in traces:
    if t not in all_traces:
        print(f"Trace {t} is not a valid trace.")
        exit(1)


def trace(i):
    subprocess.run(["make", "cleand"], check=True, capture_output=not args.verbose)
    subprocess.run(["make", "init"], check=True, capture_output=not args.verbose)
    start = timer()
    result = subprocess.run(
        ["bash", osp.join(traces_folder, f"{i}.sh")],
        capture_output=not args.verbose,
        text=True,
        cwd=cwd,
    )
    if result.returncode != 0:
        print(
            "生成标准答案时出错，请检查是否是 mnt/ 没有正确被清空或者卸载，如果发现是BUG，请联系助教"
        )
        print(f"Trace {i} failed with error code {result.returncode}")
        print(result.stderr)
        return (False, "", "", 0, 0)
    end = timer()
    time_ref = end - start
    output_ref = result.stdout if not args.verbose else ""
    if i.startswith("p"):
        start = timer()
        result2 = subprocess.run(
            ["bash", osp.join(traces_folder, f"{i}.sh")],
            capture_output=not args.verbose,
            text=True,
            cwd=cwd,
        )
        if result2.returncode != 0:
            print(
                "生成标准答案时出错，请检查是否是 mnt/ 没有正确被清空或者卸载，如果发现是BUG，请联系助教"
            )
            print(f"Trace {i} failed with error code {result2.returncode}")
            print(result2.stderr)
            return (False, "", "", 0, 0)
        end = timer()
        time_ref2 = end - start
        time_ref += time_ref2
        output_ref2 = result2.stdout if not args.verbose else ""
        output_ref += output_ref2

    if not args.verbose:
        with open(osp.join(traces_folder, f"{i}.ans"), "w") as f:
            f.write(output_ref)

    if args.anwser:
        return (True, output_ref, output_ref, time_ref, time_ref)

    subprocess.run(["make", "mount"], check=True, capture_output=not args.verbose)
    start = timer()
    result = subprocess.run(
        ["bash", osp.join(traces_folder, f"{i}.sh")],
        capture_output=not args.verbose,
        text=True,
        cwd=cwd,
    )
    end = timer()
    if result.returncode != 0:
        print(f"Trace {i} failed with error code {result.returncode}")
        print(result.stderr)
        return (False, output_ref, "", time_ref, 0)
    time_stud = end - start
    output_stud = result.stdout if not args.verbose else ""
    if i.startswith("p"):
        subprocess.run(
            ["make", "mount_noinit"], check=True, capture_output=not args.verbose
        )
        start = timer()
        result2 = subprocess.run(
            ["bash", osp.join(traces_folder, f"{i}.sh")],
            capture_output=not args.verbose,
            text=True,
            cwd=cwd,
        )
        if result2.returncode != 0:
            print(f"Trace {i} failed with error code {result2.returncode}")
            print(result2.stderr)
            return (False, output_ref, "", time_ref, 0)
        end = timer()
        time_stud2 = end - start
        time_stud += time_stud2
        output_stud2 = result2.stdout if not args.verbose else ""
        output_stud += output_stud2

    if args.verbose:
        passed = False
        print("Verbose mode enabled, skipping output comparison")
    else:
        with open(osp.join(traces_folder, f"{i}.out"), "w") as f:
            f.write(output_stud)
        if i.startswith("o"):
            passed = True
        else:
            passed = output_ref == output_stud
    return (passed, output_ref, output_stud, time_ref, time_stud)

def main():
    outputs = []
    total_passed = 0
    open_traces = 0
    for i in traces:
        if args.anwser:
            passed, output_ref, output_stud, time_ref, time_stud = trace(i)
            if passed:
                print(
                    f"Trace {i:2} anwser generated [{time_stud:8.4f}s] [ref: {time_ref:8.4f}s] [{time_stud/time_ref:8.2f}x]"
                )
            continue

        passed, output_ref, output_stud, time_ref, time_stud = trace(i)

        if not passed:
            output = f"Trace {i:2} failed"
            print(output)
            print(f"Expected:\n{output_ref}")
            print(f"Got:\n{output_stud}")
        else:
            output = f"Trace {i:2} passed [{time_stud:8.4f}s] [ref: {time_ref:8.4f}s] [{time_ref/time_stud*100:8.5f}%]"
            print(output)
            
        if i.startswith("o"):
            open_traces += 1
        else:
            outputs.append(output)
            total_passed += 1 if passed else 0

    normal_traces = len(traces) - open_traces
    print("\n".join(outputs))
    print(
        f"{total_passed}/{normal_traces} traces passed (excluding {open_traces} open traces)"
    )
    if args.trace is None:
        gain_points = (
            total_passed / normal_traces * POINTS
            if normal_traces > 0
            else 0
        )
        print(
            f"Total points: {gain_points:.2f}/{POINTS} ({total_passed}/{normal_traces})"
        )


if __name__ == "__main__":
    main()
