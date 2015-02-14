"""usage (yet versatile): 

    import lcmaes_interface as lib 
    myfun = lambda x: sum([xi**2 for xi in x])  # a callable accepting a list of numbers 
    res = lib.cmaes.pcmaes(lib.to_fitfunc(myfun),  
                           lib.to_params([2.1] * 10, 0.1))
    lib.plot()
    
Details: for the time being `to_params` is based on `lcmaes.make_simple_parameters`, 
but that might change in future to expose more parameters. 

"""

import lcmaes as cmaes
import cma_multiplt as cmaplt
outfile = 'lcmaes.dat'

def to_params(x0, sigma0, strategy="acmaes", outfile=outfile):
    """return parameter object instance for ``lcmaes.pcmaes``"""
    p = lcmaes.make_simple_parameters(x, sigma)
    p.set_str_algo("acmaes")
    p.set_fplot(outfile)
    return p

def to_fitfunc(f):
    """return function from callable `f`, where `f` accepts a list of numbers as input."""
    return lcmaes.fitfunc_pbf.from_callable(lambda x, n: f(x))
    
def plot(outfile=outfile):
    cmaplt(outfile)
    
