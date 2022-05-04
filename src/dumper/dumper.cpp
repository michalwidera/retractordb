#include "QStruct.h"                                        // for query, token

#include <iostream>                                         // for operator<<
#include <fstream>

#include <boost/lexical_cast.hpp>                           // for lexical_cast
#include <boost/regex.hpp>                                  // IWYU pragma: keep
#include <boost/program_options.hpp>                        // IWYU pragma: keep
#include <boost/system/error_code.hpp>

#include "config.h" // Add an automatically generated configuration file

using namespace boost;

// Object coreInstance in QStruct.cpp
extern "C" qTree coreInstance ;

//
//1. dumper.exe out_p.qry --dot >out.dot
//2. dot -Tjpg out.dot -o file.jpg
//3. start file.jgp
//
void dumpGraphiz(std::ostream &xout,
    bool bShowFileds,
    bool bShowStreamProgs,
    bool bShowTags,
    bool bShowFieldTypes)
{
    //
    //dot call commandline: dot -Tjpg filewithgraph.txt -o file.jpg
    //
    xout << "digraph structs {" << std::endl ;
    xout << " node\t[shape=record];" << std::endl ;
    xout << "" ;
    std::set <std::string> planStreamRelationsSet;
    for (auto q : coreInstance) {
        //
        // Stream presentation
        //
        xout << " " ;
        xout << q.id << "\t" ;
        xout << "[shape=record," ;
        if (q.isDeclaration())
            xout << "style=filled,fillcolor=Skyblue,color=Black," ;
        else if (q.isGenerated())
            xout << "style=filled,fillcolor=Sienna,color=Black," ;
        else
            xout << "style=\"filled\",fillcolor=Gray,color=Black," ;
        xout << "label=\"" ;
        xout << q.id ;
        xout << "\\ninterval=" << q.rInterval ;
        if (q.isDeclaration())
            xout << "\\nDeclaration" ;
        ;
        if (q.isGenerated())
            xout << "\\nAutogen" ;
        ;
        // end stream specific
        //
        // fields in stream
        //
        if (bShowFileds) {
            xout << "|";
            xout << "{" ;
            int dTag(0);
            bool isFirst(true);
            for (auto f : q.lSchema) {
                if (isFirst)
                    isFirst = false ;
                else
                    xout << "|" ;
                if (bShowTags)
                    xout << "<tag" << dTag ++ << ">" ;
                //
                // Patch on gramma problem -
                // dot program is using { as important sign - we need to convert { to <
                //
                std::string sFieldName(f.getFieldNameSet()) ;
                std::replace(sFieldName.begin(), sFieldName.end(), '{', '/');
                std::replace(sFieldName.begin(), sFieldName.end(), '}', '/');
                std::string sFieldString(f.getFieldText()) ;
                xout << sFieldString ;
                if (bShowFieldTypes)
                    switch (f.dFieldType) {
                        case field::BAD :
                            xout << "";
                            break ;
                        case field::BYTE :
                            xout << "(Byte)";
                            break ;
                        case field::INTEGER:
                            xout << "(Integer)";
                            break ;
                        case field::RATIONAL:
                            xout << "(Rational)";
                            break ;
                        default:
                            xout << "(Integer)";
                            //std::cerr << "?? " << f.dFieldType << std::endl ;
                            //throw std::invalid_argument("niezdefiniowany typ w polu");
                    }
            }
            xout << "}" ;
        } //if ( bShowFileds ) - end of fields in stream
        //
        // stream specific
        //
        xout << "\"]" ;
        xout << std::endl ;
        if (!q.isDeclaration())
            if (bShowStreamProgs) {
                xout << " prg_" ;
                xout << q.id << "\t" ;
                xout << "[shape=box,style=filled," ;
                if (q.isReductionRequired()) {
                    xout << "fillcolor=Green,color=Black," ;
                    xout << "label=\"";
                } else if (q.isGenerated()) {
                    xout << "fillcolor=Yellow,color=Black," ;
                    xout << "label=\"";
                } else {
                    //These are stream programs
                    xout << "fillcolor=Orange,color=Black," ;
                    xout << "label=\"";
                }
                for (token t : q.lProgram) {
                    xout << t.getStrTokenName() << " " ;
                    if (
                        t.getStrTokenName() != "STREAM_HASH" &&
                        t.getStrTokenName() != "STREAM_ADD" &&
                        t.getStrTokenName() != "STREAM_DEHASH_DIV" &&
                        t.getStrTokenName() != "STREAM_DEHASH_MOD"
                    )
                        xout << t.getValue() ;
                    xout << "\\n" ;
                }
                xout << "\"]" ;
                xout << std::endl ;
                //
                // Relation stream plan to stream
                //
                std::string relation(q.id + " -> "  + "prg_" + q.id) ;
                planStreamRelationsSet.insert(relation);
            }
    }
    for (auto s : planStreamRelationsSet)
        xout << s << std::endl;
    //
    // Due this variable we eleminate redundant relation on schema
    //
    std::set <std::string> streamRelationsSet;
    //
    // Programs that show build of data stream
    //
    if (bShowTags) {
        for (auto q : coreInstance) {
            int dTag(0) ;
            for (auto f : q.lSchema) {
                if (q.isDeclaration())
                    continue ;
                xout << " " ;
                xout << q.id << "_tag" << dTag << "\t" ;
                if (q.isDeclaration())
                    xout << "[shape=record,style=filled,fillcolor=Sienna,color=Black,label=\"" ;
                else
                    xout << "[shape=record,label=\"" ;
                std::string sFieldName(f.getFieldNameSet()) ;
                std::replace(sFieldName.begin(), sFieldName.end(), '{', '/');
                std::replace(sFieldName.begin(), sFieldName.end(), '}', '/');
                xout << sFieldName ;
                xout << "|" ;
                xout << "{" ;
                bool isFirst(true);
                if (q.isDeclaration())
                    xout << "Declaration" ;
                else
                    for (auto t : f.lProgram) {
                        if (isFirst)
                            isFirst = false ;
                        else
                            xout << "|" ;
                        std::string sTokenName(t.getStrTokenName()) ;
                        std::replace(sTokenName.begin(), sTokenName.end(), '{', '/');
                        std::replace(sTokenName.begin(), sTokenName.end(), '}', '/');
                        xout << sTokenName ;
                        //Token PUSH_ something has always some value on somethin
                        std::basic_string <char>::size_type idx = sTokenName.find("PUSH_");
                        if (idx != std::string::npos) {
                            xout << " " ;
                            xout << t.getValue() ;
                            //becasue after compilation disapear schema[1,2] and translate to schema & crvalue
                            if (sTokenName == "PUSH_ID") {
                                xout << "[" ;
                                if (t.getCRValue().denominator() == 1)
                                    xout << t.getCRValue().numerator() ;
                                else
                                    xout << t.getCRValue()  ;
                                xout << "]" ;
                            }
                        }
                    }
                xout << "}" ;
                xout << "\"" ;
                xout << "]" ;
                xout << std::endl ;
                //Relation1
                std::string relation(
                    q.id + ":" + "tag" + lexical_cast<std::string> (dTag) +
                    " -> " +
                    q.id + "_tag" + lexical_cast<std::string> (dTag) +
                    " [style=dotted]");
                streamRelationsSet.insert(relation);
                ++ dTag ;
            }
        }
    }
    //
    // Stream realtions
    //
    for (auto q : coreInstance) {
        for (auto t : q.lProgram) {
            if (t.getStrTokenName() == "PUSH_STREAM") {
                if (q.isDeclaration())
                    continue ;
                std::string relation(q.id + " -> " + t.getValue()) ;
                if (bShowStreamProgs)
                    relation = "prg_" + relation ;
                streamRelationsSet.insert(relation);
            }
        }
    }
    for (auto s : streamRelationsSet)
        xout << s << std::endl;
    xout << "}" << std::endl ;
}

void dumpQFieldsProgram()
{
    std::cout << std::endl ;
    std::cout << "fcnt_ref\tid_ref\ttoken\tvalue" << std::endl ;
    for (auto q : coreInstance) {
        for (auto f : q.lSchema) {
            for (auto t : f.lProgram) {
                std::cout << q.id  << "\t" ;
                std::cout << f.getFieldNameSet() << "\t" ;
                std::cout << t.getStrTokenName() << "\t" ;
                std::cout << t.getValue();
                if (t.getStrTokenName() == "PUSH_ID")
                    std::cout << "[" << t.getCRValue() << "]" ;
                std::cout << std::endl ;
            }
        }
    }
}

void dumpQFields()
{
    std::cout << std::endl ;
    std::cout << "fcnt\tid_ref\tfName" << std::endl ;
    for (auto q : coreInstance) {
        int loccnt(0);
        for (auto f : q.lSchema) {
            std::cout << ++ loccnt << "\t" ;
            std::cout << q.id  << "\t" ;
            std::cout << f.getFieldNameSet() << "\t" ;
            std::cout << std::endl ;
        }
    }
}

void dumpQPrograms()
{
    std::cout << std::endl ;
    std::cout << "qcnt\tid_ref\ttoken\tvalue" << std::endl ;
    for (auto q : coreInstance) {
        int loccnt(0);
        for (auto t : q.lProgram) {
            std::cout << ++ loccnt << "\t" ;
            std::cout << q.id << "\t" ;
            std::cout << t.getStrTokenName() << "\t" ;
            std::cout << t.getValue() ;
            std::cout << std::endl ;
        }
    }
}

void dumpQSet()
{
    std::cout << std::endl ;
    std::cout << "id\tlen prg\tlen sch\tinterval\tfilename" << std::endl ;
    for (auto q : coreInstance) {
        std::cout << q.id << "\t" ;
        std::cout << (int) q.lProgram.size() << "\t" ;
        std::cout << (int) q.lSchema.size() << "\t" ;
        std::cout << q.rInterval << "\t" ;
        std::cout << q.filename << "\t" ;
        std::cout << std::endl ;
    }
}

void dumpQHierarchy()
{
    for (auto q : coreInstance) {
        std::cout << q.id << "(" << q.rInterval << ")" ;
        if (! q.filename.empty())
            std::cout << "\t" << q.filename ;
        std::cout << std::endl ;
        for (auto t : q.lProgram)
            std::cout << "\t:- " << t.getStrTokenName() << "(" << t.getValue() << ")" << std::endl ;
        for (auto f : q.lSchema) {
            std::cout << "\t" ;
            for (auto s : f.setFieldName)
                std::cout << s << ":" ;
            std::cout << std::endl ;
            for (auto tf : f.lProgram)
                if (tf.getStrTokenName() == "PUSH_ID") {
                    std::cout << "\t\t"
                        << tf.getStrTokenName()
                        << "("
                        << tf.getValue()
                        << "["
                        <<        tf.getCRValue()
                        << "])" << std::endl ;
                } else {
                    std::cout << "\t\t"
                        << tf.getStrTokenName()
                        << "("
                        << tf.getValue()
                        << ")"
                        << std::endl ;
                }
        }
    }
}

int main(int argc, char* argv[])
{
    try {
        namespace po = boost::program_options;
        std::string sInputFile ;
        po::options_description desc("Avaiable options");
        desc.add_options()
        ("help,h", "show help options")
        ("verbose,r", "verbose mode")
        ("dot,d", "create dot file")
        ("csv,c", "create csv file")
        ("view,v", "create dot file and then call dot process")
        ("fields,f", "show fields in dot file")
        ("tags,t", "show tags in dot file")
        ("streamprogs,s", "show stream programs in dot file")
        ("sdump,p", "take as input file executor dump")
        ("leavedot,e", "dont delete temporary dot file")
        ("showtypes,w", "show field types in dot file")
        ("infile,i", po::value<std::string> (&sInputFile)->default_value("query.qry"), "name of input data file")
        ;
        po::positional_options_description p;
        p.add("infile", -1);
        po::variables_map vm;
        po::store(po::command_line_parser(argc, argv).
            options(desc).positional(p).run(), vm);
        po::notify(vm);
        if (vm.count("verbose") || vm.count("help"))
            std::cerr << argv[0] << " - qry file decoder.\n";
        if (vm.count("help")) {
            std::cout << desc ;
            std::cout << CONFIG_LINE ;
            return system::errc::success;
        }
        std::string sPlikDanych = vm["infile"].as<std::string>() ;
        //
        // Main algorithm
        //
        std::ifstream ifs(sPlikDanych.c_str(), std::ios::binary);
        boost::archive::text_iarchive ia(ifs);
        ia >> coreInstance ;
        //
        // Check if we put depended parameters
        //
        if (vm.count("tags") != 0 &&  vm.count("fields") == 0) {
            std::cerr << "Conflicting parameters" << std::endl ;
            std::cerr << "Tags are referencing fields - when you set tags, leve field in dots" << std::endl ;
            return system::errc::invalid_argument;
        }
        if (vm.count("view")) {
            const std::string sTempDotFile("temp_$$$.dot");
            const std::string sTempPngFile("temp_$$$.png");
            {
                std::ofstream ofs(sTempDotFile.c_str());
                dumpGraphiz(ofs,
                    vm.count("fields") != 0,
                    vm.count("streamprogs") != 0,
                    vm.count("tags") != 0,
                    vm.count("showtypes") != 0
                );
            }
            std::system(std::string("dot -Tpng " + sTempDotFile + " -o " + sTempPngFile).c_str());
            if (! vm.count("leavedot"))
                std::remove(sTempDotFile.c_str());
            std::cerr << "type: start " << sTempPngFile << std::endl ;
        } else if (vm.count("dot")) {
            dumpGraphiz(std::cout,
                vm.count("fields") != 0,
                vm.count("streamprogs") != 0,
                vm.count("tags") != 0,
                vm.count("showtypes") != 0
            );
        } else if (vm.count("csv")) {
            std::cerr << "Core count:" << (int) coreInstance.size() << std::endl ;
            dumpQSet() ;
            dumpQPrograms() ;
            dumpQFields() ;
            dumpQFieldsProgram() ;
        } else {
            // Default
            dumpQHierarchy() ;
        }
    } catch (std::exception &e) {
        std::cerr << e.what() << std::endl;
        return system::errc::interrupted;
    }
    return system::errc::success;
}
