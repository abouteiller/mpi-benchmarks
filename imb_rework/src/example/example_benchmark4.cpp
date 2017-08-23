#include <mpi.h>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <iostream>

#include "benchmark.h"
#include "benchmark_suites_collection.h"
#include "scope.h"
#include "utils.h"
#include "args_parser.h"

using namespace std;

namespace example_suite4 {

    #include "benchmark_suite.h"

    DECLARE_BENCHMARK_SUITE_STUFF(BS_GENERIC, example_suite4)

    // EXAMPLE 4: extended PingPong code with some extra features
    // - declare_args()/prepare() overloaded virtual function to implement some command line parameters
    // - get_parameter() overloaded virtual function implements the high-level interface 
    // to pass parameters from a suite to a benchmark
    template <> void BenchmarkSuite<BS_GENERIC>::declare_args(args_parser &parser) const {
        parser.add_option_with_defaults_vec<int>("len", "1,2,4,8").
                     set_mode(args_parser::option::APPLY_DEFAULTS_ONLY_WHEN_MISSING);
        parser.add_option_with_defaults<string>("datatype", "int").
                     set_caption("int|char");
        parser.add_option_with_defaults<int>("ncycles", 1000);
    }
    
    vector<int> len;
    MPI_Datatype datatype;
    int ncycles;

    template <> bool BenchmarkSuite<BS_GENERIC>::prepare(const args_parser &parser, const set<string> &) {
        parser.get_result_vec<int>("len", len);
        string dt = parser.get_result<string>("datatype");
        if (dt == "int") datatype = MPI_INT;
        else if (dt == "char") datatype = MPI_CHAR;
        else {
            // FIXME get rid of cout some way!
            cout << "Unknown data type in datatype option" << endl;
            return false;
        }
        ncycles = parser.get_result<int>("ncycles");
        return true;       
    }

#define HANDLE_PARAMETER(TYPE, NAME) if (key == #NAME) { \
                                        result = smart_ptr< TYPE >(&NAME); \
                                        result.detach_ptr(); }

#define GET_PARAMETER(TYPE, NAME) TYPE *p_##NAME = suite->get_parameter(#NAME).as< TYPE >(); \
                                  assert(p_##NAME != NULL); \
                                  TYPE &NAME = *p_##NAME;

    template <> any BenchmarkSuite<BS_GENERIC>::get_parameter(const string &key) {
        any result;
        HANDLE_PARAMETER(vector<int>, len);
        HANDLE_PARAMETER(MPI_Datatype, datatype);
        HANDLE_PARAMETER(int, ncycles);
        return result;
    }

    class ExampleBenchmark_4 : public Benchmark {
        std::map<int, double> results;
        char *sbuf, *rbuf;
        int np, rank;
        public:
        virtual void init() {
            GET_PARAMETER(vector<int>, len);
            GET_PARAMETER(MPI_Datatype, datatype);
            VarLenScope *sc = new VarLenScope(len);
            scope = sc;
            int idts;
            MPI_Type_size(datatype, &idts);
            rbuf = (char *)malloc((size_t)scope->get_max_len() * (size_t)idts);
            sbuf = (char *)malloc((size_t)scope->get_max_len() * (size_t)idts);
        }
        virtual void run(const scope_item &item) { 
            GET_PARAMETER(MPI_Datatype, datatype);
            GET_PARAMETER(int, ncycles);
            MPI_Comm_size(MPI_COMM_WORLD, &np);
            MPI_Comm_rank(MPI_COMM_WORLD, &rank);
            if (np < 2) {
                std::cout << get_name() << ": two or more ranks required" << std::endl;
                return;
            }
            MPI_Status stat;
            double t1 = 0, t2 = 0, time = 0;
            const int tag = 1;
            if (rank == 0) {
                t1 = MPI_Wtime();
                for(int i = 0; i < ncycles; i++) {
                    MPI_Send((char*)sbuf, item.len, datatype, 1, tag, MPI_COMM_WORLD);
                    MPI_Recv((char*)rbuf, item.len, datatype, 1, MPI_ANY_TAG, MPI_COMM_WORLD, &stat);
                }
                t2 = MPI_Wtime();
                time = (t2 - t1) / ncycles;
            } else if (rank == 1) {
                t1 = MPI_Wtime();
                for(int i = 0; i < ncycles; i++) {
                    MPI_Recv((char*)rbuf, item.len, datatype, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &stat);
                    MPI_Send((char*)sbuf, item.len, datatype, 0, tag, MPI_COMM_WORLD);
                }
                t2 = MPI_Wtime();
                time = (t2 - t1) / ncycles;
            } 
            MPI_Barrier(MPI_COMM_WORLD);
            results[item.len] = time;
        }
        virtual void finalize() { 
            if (rank == 0) {
                for (std::map<int, double>::iterator it = results.begin();
                        it != results.end(); ++it) {
                    std::cout << get_name() << ": " << "len=" << it->first << " time=" << it->second << std::endl; 
                }
            }
        }
        virtual ~ExampleBenchmark_4() {
            free(rbuf);
            free(sbuf);
        }
        DEFINE_INHERITED(ExampleBenchmark_4, BenchmarkSuite<BS_GENERIC>);
    };

    DECLARE_INHERITED(ExampleBenchmark_4, example4)
}
