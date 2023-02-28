#include "QStruct.h"
#include "SOperations.h"

#include <boost/lexical_cast.hpp>
#include <boost/regex.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/program_options.hpp>
#include <boost/system/error_code.hpp>

#include <fstream>
#include <iostream>

#include "compiler.hpp"

using boost::lexical_cast;

extern int fieldCount;

extern "C" {
    qTree coreInstance ;
}

std::vector<std::string> load_file(std::string sInputFile)
{
    vector<string> retVal;
    std::ifstream input(sInputFile.c_str(), ifstream::in);

    if (! input.is_open())
    {
        throw std::out_of_range("File not found.");
    }

    string str;

    while (getline(input, str))
    {
        boost::algorithm::trim(str);    //trailing and leading spaces need to be cut off

        if (str == "stop")              //stop command is not covered by grammar
        {
            break ;
        }

        retVal.push_back(str);
    }

    return retVal ;
}

/** This procedure computes time delays (delata) for generated streams */
string intervalCounter()
{
    while (true)
    {
        bool bOnceAgain(false);
        coreInstance.sort();

        for (auto &q : coreInstance)
        {
            if (q.lProgram.size() == 0)
            {
                continue ;    /* Declaration */
            }

            if (q.lProgram.size() == 1)
            {
                token tInstance(* (q.lProgram.begin()));
                q.rInterval = coreInstance.getDelta(tInstance.getValue()) ;
                continue ;  // Just one stream
            }

            assert(q.lProgram.size() == 3 || q.lProgram.size() == 2) ;
            // This is shit coded (these size2 i size3) and fast fixed
            bool size3 = (q.lProgram.size() == 3);
            list < token > loc = q.lProgram ;
            token t1(*loc.begin()) ;

            if (size3)
            {
                loc.pop_front();
            }

            token t2(*loc.begin()) ;
            loc.pop_front();
            token op(*loc.begin()) ;
            loc.pop_front();
            boost::rational<int> delta(-1) ;

            switch (op.getTokenCommand())
            {
                case STREAM_HASH:
                    {
                        boost::rational<int> delta1 = coreInstance.getDelta(t1.getValue()) ;
                        boost::rational<int> delta2 = coreInstance.getDelta(t2.getValue()) ;

                        if (delta1 == 0 || delta2 == 0)
                        {
                            bOnceAgain = true ;
                            continue ;
                        }
                        else
                        {
                            delta = deltaHash(delta1, delta2) ;
                        }
                    }
                    break ;

                case STREAM_DEHASH_DIV:
                    {
                        boost::rational<int> delta1 = coreInstance.getDelta(t1.getValue()) ;
                        boost::rational<int> delta2 = t2.getCRValue() ; //There is no second stream - just fraction argument
                        assert(delta2 != 0);

                        if (delta1 == 0)
                        {
                            bOnceAgain = true ;
                            continue ;
                        }
                        else
                        {
                            //           D_c * D_b
                            //   D_a = --------------
                            //         abs(D_c - D_b)
                            delta = deltaDivMod(delta1, delta2) ;
                        }

                        if (delta1 > delta)
                        {
                            throw std::out_of_range("You cannot make faster div from slower source");
                        }
                    }
                    break ;

                case STREAM_DEHASH_MOD:
                    {
                        boost::rational<int> delta1 = coreInstance.getDelta(t1.getValue()) ;
                        boost::rational<int> delta2 = t2.getCRValue() ;
                        assert(delta2 != 0);

                        if (delta1 == 0)
                        {
                            bOnceAgain = true ;
                            continue ;
                        }
                        else
                        {
                            //           D_c * D_a
                            //   D_b = --------------
                            //         abs(D_c - D_a)
                            delta = deltaDivMod(delta2, delta1) ;
                        }

                        if (delta1 > delta)
                        {
                            throw std::out_of_range("You cannot make faster mod from slower source");
                        }
                    }
                    break ;

                case STREAM_SUBSTRACT:
                    {
                        boost::rational<int> delta1 = coreInstance.getDelta(t1.getValue()) ;

                        if (delta1 == 0)
                        {
                            bOnceAgain = true ;
                            continue ;
                        }
                        else
                        {
                            delta = deltaSubstract(delta1) ;
                        }
                    }
                    break ;

                case STREAM_ADD:
                    {
                        boost::rational<int>delta1 = coreInstance.getDelta(t1.getValue()) ;
                        boost::rational<int>delta2 = coreInstance.getDelta(t2.getValue()) ;

                        if (delta1 == 0 || delta2 == 0)
                        {
                            bOnceAgain = true ;
                            continue ;
                        }
                        else
                        {
                            delta = deltaAdd(delta1, delta2) ;
                        }
                    }
                    break ;

                case STREAM_AVG:
                case STREAM_MIN:
                case STREAM_MAX:
                case STREAM_SUM:

                // Delta UNCHANGED ! (like time move)
                case STREAM_TIMEMOVE:
                    {
                        boost::rational<int>delta1 = coreInstance.getDelta(t1.getValue()) ;

                        if (delta1 == 0)
                        {
                            bOnceAgain = true ;
                            continue ;
                        }

                        delta = deltaTimemove(delta1, 0) ;
                    }
                    break ;

                case STREAM_AGSE:
                    {
                        // ->>> check need
                        // core1@(5,3) ->
                        // push_stream core0 -> deltaSrc
                        // push_val 5 -> size_of_window
                        // stream agse 3 -> step_of_window
                        boost::rational<int> deltaSrc = coreInstance.getDelta(t1.getValue()) ;
                        boost::rational<int> schemaSizeSrc = getQuery(t1.getValue()).getFieldNamesList().size() ;
                        boost::rational<int> step = abs(t2.getCRValue());
                        boost::rational<int> windowSize  = abs(op.getCRValue()) ;
                        assert(windowSize > 0);
                        assert(step > 0);

                        if (t2.getCRValue() < 0)     // windowSize < 0
                        {
                            delta = deltaSrc ;
                            delta /= schemaSizeSrc ;
                            delta *= windowSize ;
                            delta /= step ;
                        }
                        else
                        {
                            delta = (deltaSrc / schemaSizeSrc) * step ;
                        }
                    }
                    break ;

                default:
                    cerr << op.getStrTokenName() << " ";
                    cerr << op.getCRValue() << " ";
                    cerr << op.getValue() << " ";
                    cerr << endl ;
                    throw std::out_of_range("Undefined token/command on list");
            } //switch ( op.getTokenCommand() )

            assert(delta != -1);
            q.rInterval = delta;  //There is estabilished delta value - return value
        }  //BOOST_FOREACH ( query & q , coreInstance )

        if (! bOnceAgain)
        {
            break ;
        }
        else
        {
            coreInstance.sort();
        }
    } //while(true)

    coreInstance.sort();
    return string("OK") ;
}

string generateStreamName(string sName1, string sName2, command_id cmd)
{
    string sOperation("undefinied");

    switch (cmd)
    {
#define DEF_CASE( _A_ ) case _A_ : sOperation = #_A_ ; break ;
#include "tokendefset.h"
#undef DEF_CASE
    }

    if (sName2 == "")
    {
        return sOperation + string("_") + sName1 ;
    }
    else
    {
        return sOperation + string("_") + sName2 + string("_") + sName1 ;
    }
}

/* Goal of this procedure is to provide stream to canonincal form
TODO: Stream_MAX,MIN,AVG...
*/
string simplifyLProgram()
{
    coreInstance.sort();

    for (
        qTree::iterator it = coreInstance.begin() ;
        it != coreInstance.end();
        ++it
    )
    {
        //Agse pahse optization
        token t0, t1, t2 ;

        for (
            list < token >::iterator it2 = (*it).lProgram.begin() ;
            it2 != (*it).lProgram.end() ;
            it2 ++
        )
        {
            t0 = t1 ;
            t1 = t2 ;
            t2 = (*it2);

            if (t2.getStrTokenName() == "STREAM_AGSE" &&
                t1.getStrTokenName() == "PUSH_VAL" &&
                t0.getStrTokenName() == "PUSH_VAL"
            )
            {
                token newVal(t2.getTokenCommand(), t1.getCRValue(), t1.getValue());
                it2 = (*it).lProgram.erase(it2);
                it2 -- ;
                it2 = (*it).lProgram.erase(it2);
                (*it).lProgram.insert(it2, newVal);
                it2 = (*it).lProgram.begin() ;
                break;
            }
        }

        //Otimization phase 2
        if ((*it).isReductionRequired())
        {
            for (
                list < token >::iterator it2 = (*it).lProgram.begin() ;
                it2 != (*it).lProgram.end() ;
                it2 ++
            )
            {
                if (
                    (*it2).getStrTokenName() == "STREAM_TIMEMOVE" ||
                    (*it2).getStrTokenName() == "STREAM_SUBSTRACT"
                )
                {
                    query newQuery ;
                    string arg1 ;   //< Name of argument operation
                    command_id cmd ;
                    string affectedField ;
                    {
                        token newVal(*it2);
                        newQuery.lProgram.push_front(newVal);
                        cmd = (*it2).getTokenCommand() ;
                        it2 = (*it).lProgram.erase(it2);
                        it2 -- ;
                    }
                    {
                        token newVal(*it2);
                        newQuery.lProgram.push_front(newVal);
                        stringstream s ;
                        s << (*it2).getValue() ;
                        arg1 =  string(s.str()) ;
                        affectedField = (*it2).getValue() ;
                        it2 = (*it).lProgram.erase(it2);
                        it2 -- ;
                    }
                    it2 ++ ;
                    list < token > lSchema;
                    lSchema.push_back(token(PUSH_TSCAN));
                    newQuery.lSchema.push_back(field("*", lSchema, field::BAD, "*")) ;
                    newQuery.id = generateStreamName(arg1, "", cmd);
                    (*it).lProgram.insert(it2, token(PUSH_STREAM, newQuery.id));
                    coreInstance.push_back(newQuery);    //After this instruction it loses context
                    it = coreInstance.begin() ;
                    break ;
                }
                else if (
                    (*it2).getStrTokenName() != "PUSH_STREAM" &&
                    (*it2).getStrTokenName() != "PUSH_VAL"
                )
                {
                    query newQuery ;
                    string
                    arg1,   //< Name of first opeartion arguemnt
                    arg2 ;  //< Name of second opeartion arguemnt
                    command_id cmd ;
                    {
                        token newVal(*it2);
                        newQuery.lProgram.push_front(newVal);
                        cmd = (*it2).getTokenCommand() ;
                        it2 = (*it).lProgram.erase(it2);
                        it2 -- ;
                    }
                    {
                        token newVal(*it2);
                        newQuery.lProgram.push_front(newVal);
                        stringstream s ;
                        s << (*it2).getValue() ;
                        arg1 =  string(s.str()) ;
                        it2 = (*it).lProgram.erase(it2);
                        it2 -- ;
                    }
                    {
                        token newVal(*it2);
                        newQuery.lProgram.push_front(newVal);
                        stringstream s ;
                        s << (*it2).getValue() ;
                        arg2 =  string(s.str()) ;
                        it2 = (*it).lProgram.erase(it2);
                        it2 -- ;
                    }
                    it2 ++ ;
                    list < token > lSchema;
                    lSchema.push_back(token(PUSH_TSCAN));
                    newQuery.lSchema.push_back(field("*", lSchema, field::BAD, "*")) ;
                    newQuery.id = generateStreamName(arg1, arg2, cmd);
                    (*it).lProgram.insert(it2, token(PUSH_STREAM, newQuery.id));
                    coreInstance.push_back(newQuery);
                    it = coreInstance.begin() ;
                    break ;
                }   //Endif
            }   //Endfor
        }  //Endif
    }  //Endfor

    return string("OK") ;
}

//Goal of this procedure is to unroll schema bsased of given command
list < field > combine(string sName1, string sName2, token cmd_token)
{
    list < field > lRetVal ;
    command_id cmd = cmd_token.getTokenCommand() ;

    //Merge of schemas for junction of hash type
    if (cmd == STREAM_HASH)
    {
        if (
            getQuery(sName1).lSchema.size() !=
            getQuery(sName2).lSchema.size()
        )
        {
            throw std::invalid_argument("Hash operation needs same schemas on arguments stream");
        }

        lRetVal = getQuery(sName1).lSchema ;
        //check this!!!
        //        list < field >::iterator it = lRetVal.begin() ;
        //        BOOST_FOREACH ( field f1 , getQuery( sName1 ).lSchema )
        //        {
        //            BOOST_FOREACH( const string & s , f1.setFieldName )
        //                    (*it).setFieldName.insert( s ) ;
        //            it ++ ;
        //        }
    }
    else if (cmd == STREAM_DEHASH_DIV)
    {
        lRetVal = getQuery(sName1).lSchema ;
    }
    else if (cmd == STREAM_DEHASH_MOD)
    {
        lRetVal = getQuery(sName1).lSchema ;
    }
    else if (cmd == STREAM_ADD)
    {
        int i = 0;

        for (auto f : getQuery(sName1).lSchema)
        {
            field intf;
            intf.setFieldName.insert("Field_" + boost::lexical_cast<std::string> (fieldCount++));
            intf.lProgram.push_front(token(PUSH_ID, boost::rational<int> (i++), sName1));
            lRetVal.push_back(intf);
        }

        i = 0;

        for (auto f : getQuery(sName2).lSchema)
        {
            field intf ;
            intf.setFieldName.insert("Field_" + boost::lexical_cast<std::string> (fieldCount++));
            intf.lProgram.push_front(token(PUSH_ID, boost::rational<int> (i++), sName2));
            lRetVal.push_back(intf);
        }

        return lRetVal ;
    }
    else if (cmd == STREAM_SUBSTRACT)
    {
        lRetVal = getQuery(sName1).lSchema ;
    }
    else if (cmd == STREAM_TIMEMOVE)
    {
        lRetVal = getQuery(sName1).lSchema ;
    }
    else if (cmd == STREAM_AVG)
    {
        field intf ;
        intf.setFieldName.insert("avg");
        intf.dFieldType = field::RATIONAL ;
        intf.lProgram.push_front(token(PUSH_ID, sName1));
        lRetVal.push_back(intf);
        return lRetVal ;
    }
    else if (cmd == STREAM_MIN)
    {
        field intf ;
        intf.setFieldName.insert("min");
        intf.dFieldType = field::RATIONAL ;
        intf.lProgram.push_front(token(PUSH_ID, sName1));
        lRetVal.push_back(intf);
        return lRetVal ;
    }
    else if (cmd == STREAM_MAX)
    {
        field intf ;
        intf.setFieldName.insert("max");
        intf.dFieldType = field::RATIONAL ;
        intf.lProgram.push_front(token(PUSH_ID, sName1));
        lRetVal.push_back(intf);
        return lRetVal ;
    }
    else if (cmd == STREAM_SUM)
    {
        field intf ;
        intf.setFieldName.insert("sum");
        intf.dFieldType = field::RATIONAL ;
        intf.lProgram.push_front(token(PUSH_ID, sName1));
        lRetVal.push_back(intf);
        return lRetVal ;
    }
    else if (cmd == STREAM_AGSE)
    {
        //Unrolling schema for agse - discussion needed if we need do that this way
        list < field > schema ;
        int windowSize = abs(rational_cast<int> (cmd_token.getCRValue())) ;

        //If winows is negative - reverted schema
        if (rational_cast<int> (cmd_token.getCRValue()) > 0)
        {
            for (int i = 0 ; i < windowSize ; i++)
            {
                field intf ;
                intf.setFieldName.insert(sName1 + "_" + lexical_cast<string> (i));
                intf.dFieldType = field::BAD ;
                schema.push_back(intf);
            }
        }
        else
        {
            for (int i = windowSize - 1 ; i >= 0  ; i--)
            {
                field intf ;
                intf.setFieldName.insert(sName1 + "_" + lexical_cast<string> (i));
                intf.dFieldType = field::BAD ;
                schema.push_back(intf);
            }
        }

        lRetVal = schema ;
    }
    else
    {
        throw std::invalid_argument("Command stack hits undefinied opeation");
    }

    //Here are added to fields execution methods
    //by reference to schema position
    int offset(0) ;

    for (auto &f : lRetVal)
    {
        stringstream s ;

        if (cmd == STREAM_HASH)
        {
            s << sName1 ;
        }
        else if (cmd == STREAM_TIMEMOVE)
        {
            s << sName1 ;
        }
        else if (cmd == STREAM_AGSE)
        {
            s << sName1 ;
        }
        else
        {
            s << sName1 ; //generateStreamName( sName2, sName1, cmd ) ;
        }

        s << "[" ;
        s << offset ++ ;
        s << "]" ;

        if (f.lProgram.size() > 0)
        {
            f.lProgram.pop_front();
        }

        f.lProgram.push_front(token(PUSH_ID2, s.str()));
    }

    return lRetVal ;
}

//goal of this procedure is setup of all possible fields name and unroll *
//unfortunatlle algorithm if broken - because does not search backward but next by next
//and some * can be process which have arguments appear as two asterixe
//In such case unrool does not appear and algorithm gets shitin-shitout

string prepareFields()
{
    coreInstance.tsort();

    for (auto &q : coreInstance)
    {
        for (auto &t : q.lProgram)
        {
            assert(q.lProgram.size() < 4) ;
            //fail of this asseration means that all streams are
            //after otimization alerady
            vector<list<field>::iterator> eraseList;
            list<field>::iterator it = q.lSchema.begin();

            for (auto &f : q.lSchema)
            {
                if (f.getFirstFieldToken().getTokenCommand() == PUSH_TSCAN)
                {
                    //found! - and now unroll
                    if (q.lProgram.size() == 1)
                    {
                        //we assure that on and only token is push_stream
                        assert((* q.lProgram.begin()).getTokenCommand() == PUSH_STREAM);
                        auto nameOfscanningTable = (* q.lProgram.begin()).getValue();
                        //Remove of TSCAN
                        eraseList.push_back(it);
                        //q.lSchema =  getQuery(t.getValue()).lSchema;
                        //copy list of fields from one to another
                        int filedPosition = 0;

                        for (auto s : getQuery(t.getValue()).lSchema)
                        {
                            list < token > newLProgram ;
                            newLProgram.push_back(token(PUSH_ID, boost::rational<int> (filedPosition++), nameOfscanningTable));
                            string name = "Field_" + boost::lexical_cast<std::string> (fieldCount++) ;
                            q.lSchema.push_back(field(name, newLProgram, field::INTEGER, ""));
                        }

                        break ;
                    }

                    if (q.lProgram.size() == 3)
                    {
                        list < token >::iterator eIt = q.lProgram.begin();
                        string sName1  = (* eIt ++).getValue() ;
                        string sName2  = (* eIt ++).getValue() ;
                        token cmd = (* eIt ++) ;
                        q.lSchema = combine(sName1, sName2, cmd) ;
                        break ;
                    }

                    if (q.lProgram.size() == 2)
                    {
                        list < token >::iterator eIt = q.lProgram.begin();
                        string sName1  = (* eIt ++).getValue() ;
                        token cmd = (* eIt ++) ;
                        q.lSchema = combine(sName1, "", cmd) ;
                        break ;
                    }
                }

                it ++;
            }

            for (auto eraseIt : eraseList)
            {
                q.lSchema.erase(eraseIt);
            }
        }
    }

    coreInstance.sort();
    return string("OK") ;
}

void replaceAll(std::string &str, const std::string &from, const std::string &to)
{
    if (from.empty())
    {
        return;
    }

    size_t start_pos = 0;

    while ((start_pos = str.find(from, start_pos)) != std::string::npos)
    {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
    }
}

/* If in query plan is PUSH_IDX it means that we need to duplicate [_] */
string replicateIDX()
{
    for (auto &q : coreInstance)      // for each query
    {
        for (auto &f : q.lSchema)      // for each field in query
        {
            string schemaName = "" ;
            bool found(false);

            for (auto &t : f.lProgram)      // for each token in query field
            {
                if (t.getTokenCommand() == PUSH_IDX)
                {
                    if (found)
                    {
                        if (q.lSchema.size() != 1)
                        {
                            // if _ exist then only one filed can be in query
                            throw std::out_of_range("Only one _ attribute can be in query");
                        }
                    }

                    found = true ;
                    schemaName = t.getValue();
                }
            }

            if (found)
            {
                const int fieldSize =  getQuery(schemaName).lSchema.size() ;
                assert(fieldSize > 0);
                list < field > oldFieldSet = q.lSchema ;
                assert(oldFieldSet.size() == 1);
                field oldField = * q.lSchema.begin() ;
                q.lSchema.clear();

                for (int i = 0 ; i < fieldSize ; i ++)
                {
                    list < token > newLProgram ;

                    for (auto &t : oldField.lProgram)
                    {
                        if (t.getTokenCommand() == PUSH_IDX)
                        {
                            newLProgram.push_back(token(PUSH_ID, boost::rational<int> (i), t.getValue()));
                        }
                        else
                        {
                            newLProgram.push_back(token(t.getTokenCommand(), t.getCRValue(), t.getValue()));
                        }
                    }

                    string toRepalce = oldField.getFieldText() ;
                    replaceAll(toRepalce, "_", lexical_cast<string> (i)) ;
                    field newField(schemaName + "_" + lexical_cast<string> (i), newLProgram, oldField.dFieldType, toRepalce);
                    q.lSchema.push_back(newField);
                }

                assert(q.lSchema.size() == getQuery(schemaName).lSchema.size());
                break ;
            }
        }
    }

    return string("OK");
}

/* Purpose of this function is to translate all refrences to fields
to form schema_name[postion, time_offset]
Command method of presentation aims simple data processing
Aim of this procedure is change all of push_idXXX to push_id
note that push_id is closest to push_id4
push_idXXX is searched in all stream program after reduction
*/
string convertReferences()
{
    boost::regex xprFieldId5("(\\w*)\\[(\\d*)\\]\\[(\\d*)\\]");    //something[1][1]
    boost::regex xprFieldId4("(\\w*)\\[(\\d*)\\,(\\d*)\\]");    //something[1,1]
    boost::regex xprFieldId2("(\\w*)\\[(\\d*)\\]");         //something[1]
    boost::regex xprFieldIdX("(\\w*)\\[_]");                //something[_]
    boost::regex xprFieldId1("(\\w*).(\\w*)");              //something.in_schema
    boost::regex xprFieldId3("(\\w*)");                     //field_of_corn

    for (auto &q : coreInstance)      // for each query
    {
        assert(! q.isReductionRequired());

        for (auto &f : q.lSchema)         // for each field in query
        {
            for (auto &t : f.lProgram)      // for each token in query field
            {
                const command_id cmd(t.getTokenCommand());
                const string text(t.getValue());
                boost::cmatch what;

                switch (cmd)
                {
                    case PUSH_ID1:
                        if (regex_search(text.c_str(), what, xprFieldId1))
                        {
                            assert(what.size() == 3);
                            const string schema(what[1]);
                            const string field(what[2]);

                            //aim of this procedure is found scheamt, next field in schema
                            //and then insert
                            for (auto &q1 : coreInstance)
                            {
                                if (q1.id == schema)
                                {
                                    int offset1(0);

                                    for (auto &f1 : q1.lSchema)
                                    {
                                        if (f1.setFieldName.find(field) != f1.setFieldName.end())
                                        {
                                            t = token(PUSH_ID, boost::rational<int> (offset1), schema) ;
                                            break ;
                                        }
                                        else
                                        {
                                            offset1 ++ ;
                                        }
                                    }

                                    if (offset1 == q1.lSchema.size())
                                    {
                                        throw std::out_of_range("Failure during refernece converstion - schema exist, no fields");
                                    }

                                    break ;
                                }
                            }
                        }
                        else
                        {
                            throw std::out_of_range("No mach on type conversion ID1");
                        }

                        break ;

                    case PUSH_IDX:
                        if (regex_search(text.c_str(), what, xprFieldIdX))
                        {
                            assert(what.size() == 2);
                            const string schema(what[1]);
                            t = token(PUSH_IDX, boost::rational<int> (0), schema) ;
                        }
                        else
                        {
                            throw std::out_of_range("No mach on type conversion IDX");
                        }

                        break ;

                    case PUSH_ID2:
                        if (regex_search(text.c_str(), what, xprFieldId2))
                        {
                            assert(what.size() == 3);
                            const string schema(what[1]);
                            const string sOffset1(what[2]);
                            const int offset1(atoi(sOffset1.c_str())) ;
                            t = token(PUSH_ID, boost::rational<int> (offset1), schema) ;
                        }
                        else
                        {
                            cerr << xprFieldId2 << endl ;
                            cerr << text.c_str() << endl ;
                            throw std::out_of_range("No mach on type conversion ID2");
                        }

                        break ;

                    case PUSH_ID3:
                        if (regex_search(text.c_str(), what, xprFieldId3))
                        {
                            assert(what.size() == 2);
                            const string field(what[1]);
                            list < token >::iterator eIt = q.lProgram.begin();
                            string schema1, schema2 ;
                            command_id cmd ;
                            query* pQ1(NULL), *pQ2(NULL);

                            if (q.lProgram.size() == 1)
                            {
                                schema1  = (* eIt).getValue() ;         //push stream - stream
                                cmd = (* eIt).getTokenCommand() ;
                                assert(cmd == PUSH_STREAM) ;
                                pQ1 = & getQuery(schema1) ;
                            }

                            if (q.lProgram.size() == 2)
                            {
                                schema1  = (* eIt ++).getValue() ;      //push stream - stream
                                cmd = (* eIt).getTokenCommand() ;       //comand
                                pQ1 = & getQuery(schema1) ;
                            }

                            if (q.lProgram.size() == 3)
                            {
                                schema1  = (* eIt ++).getValue() ;      //push stream - stream
                                schema2  = (* eIt ++).getValue() ;      //push stream - stream
                                cmd = (* eIt).getTokenCommand() ;       //comand
                                pQ1 = & getQuery(schema1) ;
                                pQ2 = & getQuery(schema2) ;
                            }

                            bool bFieldFound(false);
                            int offset1(0);

                            if (pQ1 != NULL)
                            {
                                offset1 = 0;

                                for (auto &f1 : (*pQ1).lSchema)
                                {
                                    if (f1.setFieldName.find(field) != f1.setFieldName.end())
                                    {
                                        t = token(PUSH_ID, boost::rational<int> (offset1), schema1) ;
                                        bFieldFound = true ;
                                    }

                                    offset1 ++ ;
                                }
                            }

                            if (pQ2 != NULL && bFieldFound == false)
                            {
                                offset1 = 0 ;

                                for (auto &f2 : (*pQ2).lSchema)
                                {
                                    if (f2.setFieldName.find(field) != f2.setFieldName.end())
                                    {
                                        t = token(PUSH_ID, boost::rational<int> (offset1), schema2) ;
                                        bFieldFound = true ;
                                    }

                                    offset1 ++ ;
                                }
                            }

                            if (bFieldFound == false)
                            {
                                throw std::logic_error("No field of given name in stream schema ID3");
                            }
                        }
                        else
                        {
                            throw std::out_of_range("No mach on type conversion ID3");
                        }

                        break ;

                    case PUSH_ID4:
                    case PUSH_ID5:
                        if (regex_search(text.c_str(), what, xprFieldId4) ||
                            regex_search(text.c_str(), what, xprFieldId5)
                        )
                        {
                            assert(what.size() == 4);
                            const string schema(what[1]);
                            const string sOffset1(what[2]);
                            const string sOffset2(what[3]);
                            const int offset1(atoi(sOffset1.c_str())) ;
                            const int offset2(atoi(sOffset2.c_str())) ;
                            bool foundSchema(false);

                            for (auto &q : coreInstance)
                            {
                                if (q.id  == schema)
                                {
                                    foundSchema = true ;
                                    break ;
                                }
                            }

                            if (!foundSchema)
                            {
                                throw std::logic_error("Field calls nonexist schema - config.log (-g)");
                            }

                            t = token(PUSH_ID, boost::rational<int> (offset1 + offset2 * q.lSchema.size()), schema) ;
                        }
                        else
                        {
                            throw std::out_of_range("No mach on type conversion ID4");
                        }

                        break ;
                }
            }
        }
    }

    return string("OK");
}

void dumpInstance(std::string sOutFile)
{
    //These object must be const
    const qTree coreInstance2(coreInstance) ;
    std::ofstream ofs(sOutFile.c_str());
    boost::archive::text_oarchive oa(ofs);
    oa << coreInstance2 ;
}
