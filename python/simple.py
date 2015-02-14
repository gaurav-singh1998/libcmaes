import lcmaes
import cma_multiplt as lcmaplt

# input parameters for a 10-D problem
x = [10] * 10
sigma = 0.1
outfile = 'lcmaes.dat'

p = lcmaes.make_simple_parameters(x, sigma)
p.set_str_algo("acmaes")
p.set_fplot(outfile)

# objective function.
def nfitfunc(x, n):
    assert len(x) == n  # should not be necessary
    return sum([y**2 for y in x])
    
# pass the function and parameters to cmaes, run optimization and collect solution object.
cmasols = lcmaes.pcmaes(lcmaes.fitfunc_pbf.from_callable(nfitfunc), p)

# visualize results
lcmaplt.plot(outfile)
