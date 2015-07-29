package.path  = "./luaclie/src/?.lua;" .. package.path
package.cpath = "./luaclie/?.so;"  .. package.path

require("luaclie")
