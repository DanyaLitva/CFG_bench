#include "config.h"

char *configs_rdf[] = {"data/graphs/rdf/go_hierarchy.g,data/grammars/"
                       "nested_parentheses_subClassOf_type.cnf",
                       "data/graphs/rdf/taxonomy.g,data/grammars/"
                       "nested_parentheses_subClassOf_type.cnf",
                       "data/graphs/rdf/eclass.g,data/grammars/"
                       "nested_parentheses_subClassOf_type.cnf",
                       "data/graphs/rdf/go.g,data/grammars/"
                       "nested_parentheses_subClassOf_type.cnf",
                       "data/graphs/rdf/taxonomy_hierarchy.g,data/grammars/"
                       "nested_parentheses_subClassOf_type.cnf",
                       NULL};

char *configs_java[] = {
    "data/graphs/java/eclipse.g,data/grammars/java_points_to.cnf",
    "data/graphs/java/lusearch.g,data/grammars/java_points_to.cnf",
    "data/graphs/java/luindex.g,data/grammars/java_points_to.cnf",
    "data/graphs/java/commons_io.g,data/grammars/java_points_to.cnf",
    "data/graphs/java/sunflow.g,data/grammars/java_points_to.cnf",
    NULL};

char *configs_c_alias[] = {
    "data/graphs/c_alias/init.g,data/grammars/c_alias.cnf",
    "data/graphs/c_alias/block.g,data/grammars/c_alias.cnf",
    "data/graphs/c_alias/fs.g,data/grammars/c_alias.cnf",
    "data/graphs/c_alias/ipc.g,data/grammars/c_alias.cnf",
    "data/graphs/c_alias/lib.g,data/grammars/c_alias.cnf",
    "data/graphs/c_alias/mm.g,data/grammars/c_alias.cnf",
    "data/graphs/c_alias/net.g,data/grammars/c_alias.cnf",
    "data/graphs/c_alias/security.g,data/grammars/c_alias.cnf",
    "data/graphs/c_alias/sound.g,data/grammars/c_alias.cnf",
    "data/graphs/c_alias/arch.g,data/grammars/c_alias.cnf",
    "data/graphs/c_alias/crypto.g,data/grammars/c_alias.cnf",
    "data/graphs/c_alias/drivers.g,data/grammars/c_alias.cnf",
    "data/graphs/c_alias/kernel.g,data/grammars/c_alias.cnf",
    "data/graphs/c_alias/postgre.g,data/grammars/c_alias.cnf",
    "data/graphs/c_alias/apache.g,data/grammars/c_alias.cnf",
    NULL};

char *configs_vf[] = {"data/graphs/vf/xz.g,data/grammars/vf.cnf",
                      "data/graphs/vf/nab.g,data/grammars/vf.cnf",
                      "data/graphs/vf/leela.g,data/grammars/vf.cnf", NULL};

char *configs_aa[] = {
                      "data/graphs/aa/cactus.g,data/grammars/aa.cnf",
                      "data/graphs/aa/imagick.g,data/grammars/aa.cnf",
                      "data/graphs/aa/leela.g,data/grammars/aa.cnf",
                      "data/graphs/aa/nab.g,data/grammars/aa.cnf",
                      "data/graphs/aa/omnetpp.g,data/grammars/aa.cnf",
                      "data/graphs/aa/parest.g,data/grammars/aa.cnf",
                      "data/graphs/aa/perlbench.g,data/grammars/aa.cnf",
                      "data/graphs/aa/povray.g,data/grammars/aa.cnf",
                      "data/graphs/aa/x264.g,data/grammars/aa.cnf",
                      "data/graphs/aa/xz.g,data/grammars/aa.cnf",
                       NULL};

char *configs_all[] = {
                        "data/graphs/java/eclipse.g,data/grammars/java_points_to.cnf",
                        "data/graphs/java/lusearch.g,data/grammars/java_points_to.cnf",
                        "data/graphs/java/luindex.g,data/grammars/java_points_to.cnf",
                        "data/graphs/java/commons_io.g,data/grammars/java_points_to.cnf",
                        "data/graphs/java/sunflow.g,data/grammars/java_points_to.cnf",

                        "data/graphs/c_alias/init.g,data/grammars/c_alias.cnf",
                        "data/graphs/c_alias/block.g,data/grammars/c_alias.cnf",
                        "data/graphs/c_alias/fs.g,data/grammars/c_alias.cnf",
                        "data/graphs/c_alias/ipc.g,data/grammars/c_alias.cnf",
                        "data/graphs/c_alias/lib.g,data/grammars/c_alias.cnf",
                        "data/graphs/c_alias/mm.g,data/grammars/c_alias.cnf",
                        "data/graphs/c_alias/net.g,data/grammars/c_alias.cnf",
                        "data/graphs/c_alias/security.g,data/grammars/c_alias.cnf",
                        "data/graphs/c_alias/sound.g,data/grammars/c_alias.cnf",
                        "data/graphs/c_alias/arch.g,data/grammars/c_alias.cnf",
                        "data/graphs/c_alias/crypto.g,data/grammars/c_alias.cnf",
                        
   // memory
                        // "data/graphs/c_alias/drivers.g,data/grammars/c_alias.cnf",
                        // "data/graphs/c_alias/kernel.g,data/grammars/c_alias.cnf",
                        // "data/graphs/c_alias/postgre.g,data/grammars/c_alias.cnf",
                        // "data/graphs/c_alias/apache.g,data/grammars/c_alias.cnf",

                        "data/graphs/vf/xz.g,data/grammars/vf.cnf",
                        "data/graphs/vf/nab.g,data/grammars/vf.cnf",
                        "data/graphs/vf/leela.g,data/grammars/vf.cnf", 
                        
    //                     "data/graphs/aa/cactus.g,data/grammars/aa.cnf",
    // // time
    //                     // "data/graphs/aa/imagick.g,data/grammars/aa.cnf",
    //                     "data/graphs/aa/leela.g,data/grammars/aa.cnf",
    //                     "data/graphs/aa/nab.g,data/grammars/aa.cnf",
    // // time
    //                     // "data/graphs/aa/omnetpp.g,data/grammars/aa.cnf",
    // // time
    //                     // "data/graphs/aa/parest.g,data/grammars/aa.cnf",
    // // memory
    //                     // "data/graphs/aa/perlbench.g,data/grammars/aa.cnf",
    //                     "data/graphs/aa/povray.g,data/grammars/aa.cnf",
    //                     "data/graphs/aa/x264.g,data/grammars/aa.cnf",
    //                     "data/graphs/aa/xz.g,data/grammars/aa.cnf",
                        
                        "data/graphs/rdf/go_hierarchy.g,data/grammars/"
                        "nested_parentheses_subClassOf_type.cnf",
  //time
                        // "data/graphs/rdf/taxonomy.g,data/grammars/"
                        // "nested_parentheses_subClassOf_type.cnf",

                        "data/graphs/rdf/eclass.g,data/grammars/"
                        "nested_parentheses_subClassOf_type.cnf",
                        "data/graphs/rdf/go.g,data/grammars/"
                        "nested_parentheses_subClassOf_type.cnf",
  //time
                        // "data/graphs/rdf/taxonomy_hierarchy.g,data/grammars/"
                        // "nested_parentheses_subClassOf_type.cnf",
                        
                      NULL};

char *configs_my[] = {
    "data/graphs/java/sunflow.g,data/grammars/java_points_to.cnf",
    NULL};