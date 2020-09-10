Design
======

An _Enigma feature specifier_ describes features used to represent a clause.
It is a string consisting of blocks separated by ":".

Blocks
------

Each block is a collection of specific features.  Possible values in an Enigma
feature specifier are:
   
   `C`, `G`, `T`, `P`, `W`

They stand for:

* `C`: clause features
* `G`: goal (conjecture) features
* `T`: theory features
* `P`: problem features
* `W`: proof watch features

Example Enigma feature specifiers:

* `C:G:P`
* `C:T:P:W`

Block arguments
---------------

Blocks `C`, `G`, `T` take additional optional arguments to enable some
features.  The block is then written as

   `C(l,c,d,v,h,x,s,e,a)`

Arguments correspond to:

* `l`: length statistics
* `c`: symbol count statistics
* `d`: symbol depth statistics
* `v`: vertical features
* `h`: horizontal features
* `x`: variable features
* `s`: symbol features
* `e`: eprover priority/cef values
* `a`: use anonymouse feature names

When arguments are specified for block `C`, the same arguments are used for `G`
and `T` blocks without arguments.

Example Enigma feature specifiers:

* `C(l,c,v)`
* `C(x,c,s):G(v,h,s):P`
* `C(x,c,s):G:P`

Notes:

* `C()` is the same as `C`
* `C(v):G` is the same as `C(v):G(v)`

Parametric arguments
--------------------

Some block arguments are parametric and take optional paramater/value pairs.
Allowed paramaters are:

* `b`: hash base (default: 1024)
* `c`: count of variables/symbols in histograms (default: 6)
* `l`: length of vertical walks (default: 3)

Values are natural numbers.  Pairs are written as `parameter=value`, possibly
seperated by `;` and enclosed between `[` and `]`:

Examples of parametric arguments:

* `x[c=6]`: variable statistics with histograms of size 6
* `v[l=3;h=256]`: vertical walks of length 3 hashed to 256 buckets

Example parametric specifiers:

* `C(v[d=3;b=256]):G`
* `C(v[d=3;b=256],h[b=1024]):G(x[c=4]):P`

Converted to a filename:

* `C.v.d3.b256.h.b1024.G.x.c4.P`

Notes:

* Each setting of a parameter `b`, `c`, or `l` updates its default value, and
  hence any consequitive setting to the same value can be ommited.
* Thus, `C(v[b=256],h):G(h)` is the same as `C(v[b=256],h[b=256]):G([b=256])`



p(X) | q(Y,Y,Y)    # 1 0 1 ... 1 3 0

p(X,X) | q(Y,Y)

p(X,Y) | q(X,Y)



2x X
1x Y
1x Z

2 1

Y,Z
X

2








Formal
======

[number]: Number ::= 0 | 1 | 2 | ...

[value]: ValueTypes ::= 
   "b" | % base (for hashing)
   "c" | % count
   "l"   % length

[feature]: FeatureAtoms ::= 
   "l" | % length() 
   "x" | % variables(count)
   "s" | % symbols(count)
   "e" | % eprover()
   "v" | % verticals(length,base)
   "h" | % horizontals(base)
   "c" | % counts(base)
   "d" | % depths(base)<
   "a" | % anonymous 

[Block]: BlockAtoms ::= 
   "C" | % clause features
   "G" | % goal (conjecture) features
   "T" | % theory features
   "P" | % problem features (from E)
   "W"   % watchlist features

[args]: Arguments ::= 
   value number        |
   value number . args

% examples:
%    b65536
%    l3
%    c6
%    l3.b65536


[f]: Feature ::= Block_1 : Block_2 : ...


   feature           | 
   feature . args    

f 
f , 



Block ::=  
   Block(args)         |
   Block(args) : Block





