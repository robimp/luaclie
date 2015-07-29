package.path  = "./luaclie/?.lua;" .. package.path
package.cpath = "./luaclie/?.so;"  .. package.path

require("luaclie")