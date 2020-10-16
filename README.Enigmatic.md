Design
======

An _Enigma feature specifier_ describes features used to represent a clause.
It is a string consisting of blocks separated by ":".

Old Style
---------

Instead of: VHSLCXAPh


Enigmatic quick start
---------------------



Enigmatic feature specifiers
----------------------------

Enigmatic vectors represent first-order clauses, and consist of several
independent feature blocks.  Supported blocks are as follows.
   
| letter | name       | meaning
| ------ | ---------- | -----------------------------------------------
| `C`    | clause     | clause features
| `G`    | goal       | embedding of goal clauses features (TPTP types `negated_conjecture`, `conjecture`, `hypothesis`)
| `T`    | theory     | embedding of theory clauses features (other TPTP types)
| `P`    | problem    | embedding of E's problem features
| `W`    | proofwatch | Proof watch features

**Example.** To specify selected features, construct a _feature specifier_
string, by separating block letters by `:`.

* `C:G:P`
* `C:T:P:W`

### Clause block arguments ###

Clause blocks `C`, `G`, `T` take additional optional arguments to enable some
features.  The block with all the features available is written as follows.

* `C(l,p,x,s,r,v,h,c,d,a)`

Letters correspond to specific feature blocks.

| letter | meaning                                    | size       | params
| --- | --------------------------------------------- | ---------- | --------
| `l` | length statistics                             | 25         | -
| `p` | E's priority function values                  | 22         | -
| `x` | variable histograms                           | 3\**count* | `c`
| `s` | predicate/function symbol histograms          | 6\**count* | `c`
| `r` | predicate/function symbol arities             | 4\**count* | `c`
| `v` | syntax tree vertical walks of length *length* | *base*     | `b`,`l`
| `h` | syntax tree horizontal walks                  | *base*     | `b`
| `c` | positive/negative symbol counts               | *base*     | `b`
| `d` | maximal positive/negative symbol depths       | *base*     | `b`
| `a` | anonymize the symbol names                    | -          | -

__Table 1__: Clause block arguments.

### Parametric arguments ###

Some block arguments are parametric and take optional parameter/value pairs.
Parameters are of three types:

| letter | name     | meaning                        | default
| ------ | -------- | ------------------------------ | -------
| `b`    | *base*   | hashing base                   | 1024
| `c`    | *count*  | count of columns in histograms | 6
| `l`    | *length* | length of vertical walks       | 3

Allowed parameters for each feature block are in _Table 1_.
Parameters are written as follows:

* `C(x[c=10])`
* `C(l,v[l=10;b=4096]):T(l,x[c=9])` 


Enigmatic features
------------------

#### Length statistics (`l`) ####

Various 25 numeric statistics about the clause:

name | meaning
---- | -------
`len` | clause length
`lits` | number of literals
`pos` | .. of those positive 
`neg` | .. of those negative
`depth` | syntax tree depth
`width` | syntax tree width (number of leaves)
`avg_lit_depth` | average literal depth
`avg_lit_width` | .. width 
`avg_lit_len` | .. length
`pos_eqs` | number of positive equalities
`neg_eqs` | .. negative equalities
`pos_atoms` | .. positive (non-equality) atoms
`neg_atoms` | .. negative (non-equality) atoms
`vars_count` |variable count
`vars_occs` | number of variable occurrences
`vars_unique` | uniquely appearing variables
`vars_shared` | variables appearing more than once
`preds_count` | predicate symbol count
`preds_occs` | number of predicate symbol occurrences
`preds_unique` | uniquely appearing predicate symbols
`preds_shared` | predicate symbols appearing more than once
`funcs_count` | function symbol count
`funcs_occs` | number of function symbol occurrences
`funcs_unique` | uniquely appearing function symbols
`funcs_shared` | function symbols appearing more than once

Offset of these values in the vector can be obtained from the Enigmatic feature
map (viz `--output-map`).

#### E's priority function values (`p`) ####

Values from 22 selected E's clause priority functions.  For their names see the
feature map (viz `--output-map`).

#### Occurrence histograms (`x`,`s`) ####

#### Arity histograms (`r`) ####

#### Syntax tree walks (`v`,`h`)  ####

#### Count/depth statistics (`c`,`d`)  ####



### Block shortcuts ###

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

Some block arguments are parametric and take optional parameter/value pairs.
Allowed parameters are:

* `b`: hash base (default: 1024)
* `c`: count of variables/symbols in histograms (default: 6)
* `l`: length of vertical walks (default: 3)

Values are natural numbers.  Pairs are written as `parameter=value`, possibly
separated by `;` and enclosed between `[` and `]`:

Examples of parametric arguments:

* `x[c=6]`: variable statistics with histograms of size 6
* `v[l=3;b=256]`: vertical walks of length 3 hashed to 256 buckets

Example parametric specifiers:

* `C(v[l=3;b=256]):G`
* `C(v[l=3;b=256],h[b=1024]):G(x[c=4]):P`

Converted to a filename:

* `C.v.d3.b256.h.b1024.G.x.c4.P`

Notes:

* Each setting of a parameter `b`, `c`, or `l` updates its default value, and
  hence any consecutive setting to the same value can be omitted.
* Thus, `C(v[b=256],h):G(h)` is the same as `C(v[b=256],h[b=256]):G([b=256])`


Reasons
=======

* make the vectors more parametric, and allow to make more complementary models
* allow easy adding of new features
* debug memory leaks and merge the code with Stephan's repo

Developers
==========

General
-------

Extending feature vectors
-------------------------

There are three ways how to add new features into the source codes.

1. Add a value into some existing block.
2. Add a new sub-block into clause blocks (C,W,T).
3. Add a new block.

### How to: add new values into the length block (l) ###

This is the easiest, as the feature specifier syntax does not need to be
changed.  You won't be, however, able to turn the values off without turning
off the whole length block.  Adding a new value in this style affects all the
clause blocks (C, G, T).

* Add new members to store your new feature values into
  `che_enigmaticdata.h:EnigmaticClauseCell`.
* Set default values in `che_enigmaticdata.c:EnigmaticClauseReset`.
* Add new value names to `che_enigmaticdata.c:efn_lengths`.
* Increase `EFC_LEN` in `che_enigmaticdata.h` by the count of new values. 
* Compute your values somewhere in `che_enigmaticvectors.c`, for example,
  during the clause traversal in `update_clause`.  When you update
  `EnigmaticClause` (other then in `update_clause`) you must also update
  `EnigmaticClauseSet`, and vice versa.
* Update `che_enigmaticdata.c:fill_lengths` and call `set(data, key, val)` to
  set your values into the output vector.  Key will be the index in the vector.
  The offset is based on the position of your new features in `efn_lengths`.
  The value should be a float (or converted to).


### How to: add a new sub-block into the clause blocks ###

To create a new sub-block inside the clause blocks (C,G,T), the feature
specifier must be updated:

* Add a new member or members to `che_enigmaticdata.h:EnigmaticParamsCell` to
  store possible parameters of your new block.  At least add (1) a `bool` to
  specify that your new features are on/off, and (2) a `long` to store the
  offset of your values in the output vector. 
* Set the default values in `che_enigmaticdata.c:EnigmaticParamsAlloc`.
* (!) Update `che_enigmaticdata.c:EnigmaticParamsCopy`. 
* Select an unused letter to denote your new block and extend the `switch` in
  `che_enigmaticdata.c:parse_block`.  Parse the arguments there (if any) and
  set your new members in `params`.  If you have only one parameter, you can
  use `parse_one`.  In the case of more, you have to do the parsing yourself as
  in `parse_vert`.
* Set the offset correctly in `che_enigmaticdata.c:params_offsets` by calling
  `params_offset(is_on, val_ptr, len, &cur)` where:
  + `is_on` is a `bool` (if the values are on/off),
  + `val_ptr` is pointer where to store the index value,
  + `len` is the length of your new block, and
  + `cur` is just the `cur`.
  
  Call this at the appropriate place (where you want your values to appear in
  the vector).
* Update `che_enigmaticdata.c:names_clauses` to update index descriptions when
  dumping the map file.
* Update `che_enigmaticdata.c:info_suboffsets` to update sub-offset
  descriptions when dumping the map file.
* Update `che_enigmaticdata.c:info_settings` to update setting descriptions
  when dumping the map file.

Now add members to store new feature values in the block, and compute them:

* Create members in `che_enigmaticdata.h:EnigmaticClauseCell` to store your
  values.
* Allocate in `EnigmaticClauseAlloc` and update also `EnigmaticClauseFree` and
  `EnigmaticClauseReset`.
* As in the first HOWTO above, compute the values somewhere in
  `che_enigmaticvectors.c:update_clause`, or you have to update both
  `EnigmaticClause` and `EnigmaticClauseSet`.
* Update `che_enigmaticdata.c:fill_clause` to set your values into the output
  vector (see the last step in the first HOWTO).

### How to: add a new global-level block ###

This is similar to the above, and involves the following:

* Allocating members in `che_enigmaticdata.c:EnigmaticFeaturesCell` for block
  options and offsets.
* Allocating members in `che_enigmaticdata.h:EnigmaticVectorCell`, updating
  `EnigmaticVectorAlloc` and `EnigmaticVectorFree` for storing the values.
* Selecting an unused letter and extending the switch in
  `che_enigmaticdata.c:EnigmaticFeaturesParse` to parse possible block
  arguments there.  Set the offsets therein.
* Updating `che_enigmaticdata.c:PrintEnigmaticFeaturesMap`.
* Updating `che_enigmaticdata.c:PrintEnigmaticFeaturesInfo`.
* Writing a function to compute the values somewhere in
  `che_enigmaticvectors.c` (just like in the case of `EnigmaticProblem`).
* Calling that function from `SIMPLE_APPS/enigmatic-features.c` (when computing
  features), and also when initializing the clause weight functions (when
  running E).
* Updating `che_enigmaticdata.c:EnigmaticVectorFill`.





