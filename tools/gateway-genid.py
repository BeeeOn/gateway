#! /usr/bin/env python3

"""
Damm algorithm decimal check digit

For reference see http://en.wikipedia.org/wiki/Damm_algorithm
"""

damm_matrix = (
	(0, 3, 1, 7, 5, 9, 8, 6, 4, 2),
	(7, 0, 9, 2, 1, 5, 4, 8, 6, 3),
	(4, 2, 0, 6, 8, 7, 1, 3, 5, 9),
	(1, 7, 5, 0, 9, 8, 3, 4, 2, 6),
	(6, 1, 2, 3, 0, 4, 5, 9, 7, 8),
	(3, 6, 7, 4, 2, 0, 9, 5, 8, 1),
	(5, 8, 6, 9, 7, 2, 0, 1, 3, 4),
	(8, 9, 4, 5, 3, 6, 2, 0, 1, 7),
	(9, 4, 3, 8, 6, 1, 7, 2, 0, 5),
	(2, 5, 8, 1, 4, 3, 6, 7, 9, 0)
)

def damm_encode(number):
	number = str(number)
	interim = 0

	for digit in number:
		interim = damm_matrix[interim][int(digit)]

	return interim

def damm_check(number):
	return damm_encode(number) == 0

"""
Build the Gateway ID from components VERSION and VALUE as

   VERSION |   VALUE   |   DAMM
   1 digit   14 digits   1 digit

The VALUE is padded from the left by zeros.
"""
def build_gateway_id(version, value):
	id = (version * 10**14) + value
	id = (id * 10) + damm_encode(id)

	if damm_check(id):
		return id
	else:
		raise Exception("failed (%s, %s, %s), this is probably a bug"
				% (version, value, id))

"""
Generate the VALUE part at random and return the Gateway ID
of the given VERSION, random VALUE and the check DAMM digit.
"""
def random_gateway_id(version):
	import random

	value = 0
	for i in range(14):
		value *= 10
		value += random.randint(0, 9)

	return build_gateway_id(version, value)

"""
Verify the given Gateway ID. It recomputes the DAMM digit
to test the validity.
"""
def verify_gateway_id(id):
	if not damm_check(id):
		raise Exception("invalid: %s" % id)

if __name__ == '__main__':
	import argparse

	parser = argparse.ArgumentParser()
	parser.add_argument('--value', metavar='VALUE', type=int,
			default=None,
			help="avoid random generation and use the given VALUE")
	parser.add_argument('--version', metavar='VERSION', type=int,
			default=1,
			help="version of the ID format")
	parser.add_argument('--seed', metavar='SEED', type=int,
			default=None,
			help="seed for random number generator")
	parser.add_argument('--verify', metavar='ID', type=int,
			default=None,
			help="verify validity of the given ID")

	args = parser.parse_args()

	if args.version < 0 or args.version > 9:
		raise Exception("invalid version %s, must be in range 0..9"
				% args.version)
	if args.value is not None and args.verify is not None:
		raise Exception("conflicting options: --id or --verify")

	if args.seed is not None:
		random.seed(args.seed)

	if args.verify is None:
		if args.value is None:
			print(random_gateway_id(args.version))
		else:
			print(build_gateway_id(args.version, args.value))
	else:
		verify_gateway_id(args.verify)
		print("valid: %s" % args.verify)
