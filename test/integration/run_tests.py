#!/usr/bin/python3

import os
import sys
import glob

import csv
import json
import argparse
import itertools
import subprocess


def output_csv(stats, filename):
    print("Writing stats to: {}".format(filename))

    with open(filename, 'w') as f:
        writer = csv.DictWriter(f, ['test', 'processing_time'])
        writer.writeheader()
        writer.writerows(stats)


def main():

    stats = []
    passed = []
    failed = []
    skipped = []

    def pass_handler(test):
        passed.append(test)
        with open('test/stats.json') as f:
            stats.append(json.loads(f.read()))

    def skip_handler(test):
        skipped.append(test)

    def ignore_failure(test):
        failed.append(test)

    def exit_on_failure(_):
        sys.exit(1)

    parser = argparse.ArgumentParser(description="Gadgetron Integration Test",
                                     formatter_class=argparse.ArgumentDefaultsHelpFormatter)

    parser.add_argument('-G', '--gadgetron-home',
                        default=os.environ.get('GADGETRON_HOME'),
                        help="Gadgetron installation home")
    parser.add_argument('-I', '--ismrmrd-home',
                        default=os.environ.get('ISMRMRD_HOME'),
                        help="ISMRMRD installation home")

    parser.add_argument('-p', '--port', type=int, default=9003, help="Port of Gadgetron instance")
    parser.add_argument('-a', '--host', type=str, default="localhost", help="Address of (external) Gadgetron host")

    parser.add_argument('-e', '--external', action='store_const', const=['-e'], default=[],
                        help="Use external Gadgetron; don't start a new instance each test.")

    parser.add_argument('-F', '--ignore-failures', dest='failure_handler',
                        action='store_const', const=ignore_failure, default=exit_on_failure,
                        help="Ignore a failing cases; keep running tests.")
    parser.add_argument('-s', '--stats', type=str, default=None,
                        help="Output individual test stats to CSV file.")


    parser.add_argument('tests', type=str, nargs='+', help="Glob patterns; tests to run.")

    args = parser.parse_args()

    handlers = {0: pass_handler, 1: args.failure_handler, 2: skip_handler}

    tests = sorted(set(itertools.chain(*[glob.glob(pattern) for pattern in args.tests])))

    base_args = [sys.executable, 'run_gadgetron_test.py','-a',str(args.host), '-p', str(args.port)] + args.external
    if args.gadgetron_home is not None:
        base_args += ['-G',args.gadgetron_home]
    if args.ismrmrd_home is not None:
        base_args += ['-I',args.ismrmrd_home]

    for i, test in enumerate(tests, start=1):
        print("\n ----> Test {} of {}: {}\n".format(i, len(tests), test))
        proc = subprocess.run(base_args + [test])

        handlers.get(proc.returncode)(test)

    if args.stats:
        output_csv(stats, args.stats)


    if failed:
        print("\nFailed tests:")
        for test in failed:
            print("\t{}".format(test))

    print("\n{} tests passed. {} tests failed. {} tests skipped.{} tests failed.".format(len(passed), len(failed), len(skipped), len(failed)))
    print("Total processing time: {:.2f} seconds.".format(sum(stat['processing_time'] for stat in stats)))

    if(len(failed)>0):
        print("=========================================")
        for test in failed:
            print("Failed case -- %s" % test)
        print("=========================================")
        sys.exit(1)

if __name__ == '__main__':
    main()
