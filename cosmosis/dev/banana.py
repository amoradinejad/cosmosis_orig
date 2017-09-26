from __future__ import division
from past.utils import old_div
from numpy import exp

def setup(options):
	return []


def execute(block, config):
	x = block['banana', 'x']
	y = block['banana', 'y']
	z = old_div(y**2,0.6**2) + old_div((x-y**2)**2,0.1**2)
	#like = exp(-0.5*z) * exp(-0.5*x**2) * exp(-0.5*y**2) 
	like = - 0.5*z
	block['likelihoods', 'banana_like'] = like
	return 0