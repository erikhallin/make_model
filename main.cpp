#include <windows.h>
#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <fstream>
#include <time.h>
#include <unistd.h>

using namespace std;

float string_to_float(string);
int string_to_int(string);
string float_to_string(float);

int main()
{
    float version=1.06;
    cout<<"Make SAXS Model\nversion "<<version<<endl<<endl;

    cout<<"This software will take the gnom output file .out and generate a number of\nGasbor, Dammin or Dammif models.\n";
    cout<<"Requires ATSAS software (2.8.0) damsup, damsel, damaver, damfilt, damstart,\ngasbori, dammin and dammif to be able to run.\n";
    cout<<"To resume a previous run type model type and the name of the specific folder\ninstead of an input file (gasbor 20170101_010101).\n\n";
    cout<<"Made by Erik Hallin - 2017\n\n";

    //user settings
    string run_id;
    string output_dir;
    string input_file_name;
    string forced_symmetry;
    string symmetry_input;
    bool use_symmetry_for_alignment=false;
    int number_of_residues=0;
    int unit_in_use=0; //0 unknown, 1 A, 2 nm
    int model_type_to_make=0; //0 unknown, 1 gasbor, 2 dammin, 3 dammif
    int number_of_models_to_make=0;
    int run_speed=1; //1 slow, 2 fast
    bool continue_mode=false;
    string user_input_string;
    cout<<"Enter name of input file (.out file) : ";
    while(true)
    {
        getline(cin,user_input_string);
        //check if file exists
        ifstream input_test(user_input_string.c_str());
        if(input_test!=0)
        {
            input_test.close();
            input_file_name=user_input_string;
            break;
        }
        else
        {
            //quick test if .out had to be added
            string user_input_string_dotout=user_input_string;
            user_input_string_dotout.append(".out");
            ifstream input_test2(user_input_string_dotout.c_str());
            if(input_test2!=0)
            {
                input_test2.close();
                input_file_name=user_input_string_dotout;
                break;
            }

            //test for continue mode, if folder exists. Type model type run id (ex: gasbor 20170310_121212)
            string word,line,old_log_file_name;
            stringstream ss(user_input_string);
            ss>>word;
            if(word=="gasbor")
            {
                model_type_to_make=1;
                old_log_file_name="gasbor\\";
            }
            else if(word=="dammin")
            {
                model_type_to_make=2;
                old_log_file_name="dammin\\";
            }
            else if(word=="dammif")
            {
                model_type_to_make=3;
                old_log_file_name="dammif\\";
            }
            else//not valid
            {
                cout<<"That file could not be found, try again\nEnter name of input file: ";
                continue;
            }
            string temp_string_model=word;

            //check run id, if old log file exists, continue this run
            ss>>word;
            old_log_file_name.append(word+"\\log_"+word+".txt");
            ifstream old_file_test(old_log_file_name.c_str());
            if(old_file_test==0)
            {
                cout<<"That file could not be found, try again\nEnter name of input file: ";
                continue;
            }
            //file found
            run_id=word;

            cout<<"Model type: "<<temp_string_model<<endl;
            cout<<"Run id: "<<word<<endl;

            //read old values
            bool settings_found=false;
            while(true)
            {
                getline(old_file_test,line);
                if(line=="Run settings:")
                {
                    settings_found=true;
                    break;
                }
            }
            if(!settings_found)
            {
                cout<<"ERROR: Did not find previous settings\n";
                return 1;
            }
            getline(old_file_test,line);
            stringstream ss_filename(line);
            ss_filename>>word; ss_filename>>word; ss_filename>>word; ss_filename>>word; ss_filename>>word;
            input_file_name=word;
            cout<<"Input file name: "<<word<<endl;
            if(model_type_to_make==1)//only for gasbor, residue number
            {
                getline(old_file_test,line);
                stringstream ss_residue_number(line);
                ss_residue_number>>word; ss_residue_number>>word; ss_residue_number>>word; ss_residue_number>>word; ss_residue_number>>word;
                cout<<"Number of residues: "<<word<<endl;
                number_of_residues=string_to_int(word);
            }
            getline(old_file_test,line);
            stringstream ss_unit(line);
            ss_unit>>word; ss_unit>>word; ss_unit>>word;
            cout<<"Units in: "<<word<<endl;
            if(word=="Angstrom") unit_in_use=1;
            if(word=="nm") unit_in_use=2;

            getline(old_file_test,line);//already know model type, skip

            //get symmetry option
            getline(old_file_test,line);
            stringstream ss_sym(line);
            ss_sym>>word; ss_sym>>word; ss_sym>>word;
            symmetry_input=word;
            forced_symmetry=string(" -sy ");
            if(model_type_to_make==3) forced_symmetry=string(" -s ");
            forced_symmetry.append(symmetry_input);
            forced_symmetry.append(" ");

            //get sim speed
            if(model_type_to_make!=1)//not for gasbor, sim speed
            {
                getline(old_file_test,line);
                stringstream ss_residue_number(line);
                ss_residue_number>>word; ss_residue_number>>word; ss_residue_number>>word;
                if(word=="Slow") run_speed=1;
                if(word=="Fast") run_speed=2;
                cout<<"Simulation speed: "<<word<<endl;
            }

            //get model counter
            getline(old_file_test,line);
            stringstream ss_model_count(line);
            ss_model_count>>word; ss_model_count>>word; ss_model_count>>word; ss_model_count>>word; ss_model_count>>word; ss_model_count>>word;
            cout<<"Number of models to generate: "<<word<<endl;
            number_of_models_to_make=string_to_int(word);

            //get alignment option
            getline(old_file_test,line);
            stringstream ss_model_align(line);
            ss_model_align>>word; ss_model_align>>word; ss_model_align>>word; ss_model_align>>word; ss_model_align>>word;
            if(word=="Yes") use_symmetry_for_alignment=true;

            old_file_test.close();
            continue_mode=true;

            cout<<"Previous run "<<old_log_file_name<<" will be resumed\n";
            break;
        }
    }

    if(!continue_mode)
    {
        cout<<"Units in (1)Angstrom or (2)nm ? : ";
        getline(cin,user_input_string);
        if(user_input_string=="Angstrom"||user_input_string=="angstrom"||user_input_string=="a"||user_input_string=="A"||user_input_string=="1") unit_in_use=1;
        if(user_input_string=="NM"||user_input_string=="nm"||user_input_string=="Nm"||user_input_string=="nm"||user_input_string=="2") unit_in_use=2;
        while(true)
        {
            cout<<"Which model type should be generated? (1)Gasbor, (2)Dammin, (3)Dammif : ";
            getline(cin,user_input_string);
            if(user_input_string=="GASBOR"||user_input_string=="Gasbor"||user_input_string=="gasbor"||user_input_string=="G"||user_input_string=="g"||user_input_string=="1") model_type_to_make=1;
            if(user_input_string=="DAMMIN"||user_input_string=="Dammin"||user_input_string=="dammin"||user_input_string=="D"||user_input_string=="d"||user_input_string=="2") model_type_to_make=2;
            if(user_input_string=="DAMMIF"||user_input_string=="Dammif"||user_input_string=="dammif"||user_input_string=="F"||user_input_string=="f"||user_input_string=="3") model_type_to_make=3;
            if(model_type_to_make==0) cout<<"Invalid option, try again\n";
            else break;
        }
        //ask for speed
        if(model_type_to_make!=1)
        {
            cout<<"Desired simulation speed (1)Slow or (2)Fast ? : ";
            getline(cin,user_input_string);
            if(user_input_string=="FAST"||user_input_string=="Fast"||user_input_string=="fast"||user_input_string=="f"||user_input_string=="2") run_speed=2;
        }
        else//if gasbor, ask for the number of residues
        {
            cout<<"How many residues in the protein? : ";
            while(true)
            {
                getline(cin,user_input_string);
                number_of_residues=string_to_int(user_input_string);
                if(number_of_residues>0) break;
                else
                {
                    cout<<"Invalid number, try again: ";
                }
            }
        }

        //symmetry
        cout<<"Use forced symmetry? (Y/N) : ";
        getline(cin,user_input_string);
        if(user_input_string=="y"||user_input_string=="Y"||user_input_string=="YES"||user_input_string=="yes"||user_input_string=="Yes")
        {
            switch(model_type_to_make)
            {
                case 1: cout<<"Specify the point symmetry of the particle. Point groups P1, P2, ..., P19, Pn2 (n = 2, ..., 12), P23, P432 or PICO (icosahedral) are supported : "; break;
                case 2: cout<<"Specify the point symmetry of the particle. Point groups P1, P2, ..., P19, Pn2 (n = 2, ..., 12), P23, P432 or PICO (icosahedral) are supported : "; break;
                case 3: cout<<"Specify the symmetry to enforce on the particle. Any P-n-m symmetry known by DAMMIN is supported (Pn, n=1, ..., 19 and Pn2, n=2, ..., 12). Cubic and icosahedral symmetries are not available : "; break;
            }
            forced_symmetry=string(" -sy ");
            if(model_type_to_make==3) forced_symmetry=string(" -s ");
            getline(cin,user_input_string);
            symmetry_input=user_input_string;
            forced_symmetry.append(user_input_string);
            forced_symmetry.append(" ");

            //ask for alignment symmetry
            cout<<"Align models using forced symmetry? (Y/N) : ";
            getline(cin,user_input_string);
            if(user_input_string=="Yes" || user_input_string=="Y" || user_input_string=="yes" || user_input_string=="y")
                use_symmetry_for_alignment=true;
        }
        else
        {
            forced_symmetry=string(" -sy P1 ");
            if(model_type_to_make==3) forced_symmetry=string(" -s P1 ");
            symmetry_input=string("P1");
        }

        cout<<"How many models to generate? : ";
        while(true)
        {
            getline(cin,user_input_string);
            number_of_models_to_make=string_to_int(user_input_string);
            if(number_of_models_to_make>0) break;
            else
            {
                cout<<"Invalid number, try again: ";
            }
        }
    }

    //create time stamp id
    time_t timer=time(0);
    struct tm * now=localtime(&timer);
    if(!continue_mode)
    {
        run_id.append(float_to_string(now->tm_year+1900));
        if(now->tm_mon+1<10) run_id.append("0");
        run_id.append(float_to_string(now->tm_mon+1));
        if(now->tm_mday<10) run_id.append("0");
        run_id.append(float_to_string(now->tm_mday));
        run_id.append("_");
        if(now->tm_hour<10) run_id.append("0");
        run_id.append(float_to_string(now->tm_hour));
        if(now->tm_min<10) run_id.append("0");
        run_id.append(float_to_string(now->tm_min));
        if(now->tm_sec<10)  run_id.append("0");
        run_id.append(float_to_string(now->tm_sec));
    }

    //create output folder
    switch(model_type_to_make)
    {
        case 1: CreateDirectory("gasbor",NULL); output_dir.append("gasbor\\"); break;
        case 2: CreateDirectory("dammin",NULL); output_dir.append("dammin\\"); break;
        case 3: CreateDirectory("dammif",NULL); output_dir.append("dammif\\"); break;
    }
    output_dir.append(run_id);
    CreateDirectory(output_dir.c_str(),NULL);

    //copy input data to folder
    string copy_file_location=output_dir+"\\"+input_file_name;
    ofstream file_copy(copy_file_location.c_str());
    if(file_copy==0)
    {
        cout<<"ERROR: Could not create new file\n";
        return 1;
    }
    ifstream file_org(input_file_name.c_str());
    if(file_org==0)
    {
        cout<<"ERROR: Could not find input file\n";
        return 1;
    }
    string copy_line;
    while(getline(file_org,copy_line))
    {
        file_copy<<copy_line<<endl;
    }
    file_copy.close();
    file_org.close();

    //set active directory
    chdir(output_dir.c_str());
    output_dir.append("\\");

    //create log file
    string log_file_name;
    log_file_name.append("log_");
    log_file_name.append(run_id);
    log_file_name.append(".txt");
    ofstream log_file(log_file_name.c_str());
    if(log_file==0)
    {
        cout<<"ERROR: Could not create log file\n";
        log_file<<"ERROR: Could not create log file"<<endl;
        log_file.close();
        return 1;
    }
    log_file<<"Make SAXS Model\nversion "<<version<<endl;
    log_file<<now->tm_year+1900<<"-"<<now->tm_mon<<"-"<<now->tm_mday+1<<" "<<now->tm_hour<<":"<<now->tm_min<<":"<<now->tm_sec<<endl;
    log_file<<"\nRun settings:\nName of input file: "<<input_file_name<<endl;
    if(model_type_to_make==1) log_file<<"Number of dummy residues: "<<number_of_residues<<endl;
    log_file<<"Units in ";
    switch(unit_in_use)
    {
        case 0: log_file<<"???\n"; break;
        case 1: log_file<<"Angstrom\n"; break;
        case 2: log_file<<"nm\n"; break;
    }
    log_file<<"Models to make: ";
    switch(model_type_to_make)
    {
        case 0: log_file<<"???\n"; cout<<"ERROR: Bad model type\n"; log_file<<"ERROR: Bad model type\n"; log_file.close(); return 1;
        case 1: log_file<<"Gasbor\n"; break;
        case 2: log_file<<"Dammin\n"; break;
        case 3: log_file<<"Dammif\n"; break;
    }
    log_file<<"Forced symmetry: "<<symmetry_input<<endl;

    if(model_type_to_make!=1)
    {
        log_file<<"Simulation speed: ";
        if(run_speed==1) log_file<<"Slow\n";
        else log_file<<"Fast\n";
    }
    log_file<<"Number of models to generate: "<<number_of_models_to_make<<endl;

    if(use_symmetry_for_alignment) log_file<<"Align models using symmetry: Yes"<<endl;
    else                           log_file<<"Align models using symmetry: No"<<endl;

    //generate models
    timer=time(0);
    now=localtime(&timer);
    string time_stamp;
    if(now->tm_hour<10) time_stamp.append("0");
    time_stamp.append(float_to_string(now->tm_hour));
    time_stamp.append(":");
    if(now->tm_min<10) time_stamp.append("0");
    time_stamp.append(float_to_string(now->tm_min));
    time_stamp.append(":");
    if(now->tm_sec<10) time_stamp.append("0");
    time_stamp.append(float_to_string(now->tm_sec));
    cout<<"\nProcedure stated at "<<time_stamp<<endl<<endl;
    log_file<<"\nProcedure stated at "<<time_stamp<<endl<<endl;
    cout<<"Generating models...\n";
    log_file<<"Generating models..."<<endl;
    string command_line_common;//(output_dir);
    string unit_option;//only for dammin/dammif
    switch(unit_in_use)
    {
        case 1: unit_option="ANGSTROM"; break;
        case 2: unit_option="NANOMETER"; break;
    }
    string run_speed_text("SLOW");
    if(run_speed==2) run_speed_text=string("FAST");
    switch(model_type_to_make)
    {
        case 0: cout<<"ERROR: Bad model type\n"; log_file<<"ERROR: Bad model type\n"; log_file.close(); return 1;
        case 1: command_line_common.append("gasbori --un "+float_to_string(unit_in_use)+forced_symmetry+" --lo "); break;
        case 2: command_line_common.append("dammin --un "+float_to_string(unit_in_use)+forced_symmetry+" -mo "+run_speed_text+" -lo "); break;
        case 3: command_line_common.append("dammif -u "+unit_option+" -m "+run_speed_text+" "+forced_symmetry+" -p "); break;
    }
    //command_line_common.append(float_to_string(unit_in_use));
    //command_line_common.append(" --lo "/*+output_dir*/);
    for(int i=0;i<number_of_models_to_make;i++)
    {
        if(continue_mode)
        {
            //check if file already exists
            string test_file_name(float_to_string(i+1));
            if(model_type_to_make!=1) test_file_name.append("-1");
            test_file_name.append(".pdb");
            ifstream test_file(test_file_name.c_str());
            if(test_file!=0)
            {
                //file already exists
                test_file.close();

                //check log
                string log_file_name(float_to_string(i+1)+".log");
                ifstream test_log(log_file_name.c_str());
                if(test_log!=0)
                {
                    //check if run finished
                    bool run_finished=false;
                    string temp_line;
                    bool dammif_first_log_found=false;
                    while(getline(test_log,temp_line) && !run_finished)
                    {
                        //get first word in line
                        stringstream ss_log(temp_line);
                        string word;
                        ss_log>>word;
                        //gasbor and dammin writes final in the end and dammif writes log two times
                        switch(model_type_to_make)
                        {
                            case 1: if(word=="Final") run_finished=true; break;
                            case 2: if(word=="Final") run_finished=true; break;
                            case 3: if(word=="Log" && dammif_first_log_found) run_finished=true;
                                    else if(word=="Log" && !dammif_first_log_found) dammif_first_log_found=true; break;
                        }
                    }
                    if(run_finished)
                    {
                        test_log.close();
                        cout<<"Previous model "<<test_file_name<<" found\n";
                        continue;//do not rerun this file
                    }
                }
                test_log.close();
            }
        }

        string command_line_extra(float_to_string(i+1));
        command_line_extra.append(" ");
        command_line_extra.append(input_file_name);
        if(model_type_to_make==1)//only for gasbor
        {
            command_line_extra.append(" ");
            command_line_extra.append(float_to_string(number_of_residues));
        }
        string command_line=command_line_common+command_line_extra;
        //cout<<command_line<<endl;
        system(command_line.c_str());
    }
    cout<<"\nModel generation complete\n\n";
    log_file<<"...done"<<endl;

    //align
    cout<<"Aligning models...\n";
    log_file<<"Aligning models..."<<endl;
    string command_line("damsup ");
    if(use_symmetry_for_alignment)
    {
        command_line.append("-s ");
        command_line.append(symmetry_input);
        command_line.append(" ");
    }
    for(int i=0;i<number_of_models_to_make;i++)
    {
        command_line.append(/*output_dir+*/float_to_string(i+1));
        if(model_type_to_make!=1) command_line.append("-1");
        command_line.append(".pdb ");
    }
    system(command_line.c_str());
    cout<<"\nAlignment complete\n\n";
    log_file<<"...done"<<endl;;

    //find and rename origin alignment model
    for(int i=0;i<number_of_models_to_make;i++)
    {
        //check if file exists
        string file_name(float_to_string(i+1));
        if(model_type_to_make!=1) file_name.append("-1");
        file_name.append("r.pdb");
        ifstream file_test(file_name.c_str());
        if(file_test==0)
        {
            //missing file found, copy
            file_name=string(float_to_string(i+1));
            if(model_type_to_make!=1) file_name.append("-1");
            file_name.append(".pdb");
            ifstream org_file(file_name.c_str());
            if(org_file==0)
            {
                cout<<"ERROR: Aligned file missing\n";
                log_file<<"ERROR: Aligned file missing"<<endl;
                log_file.close();
                return 1;
            }
            //make new renamed file
            string new_file_name(float_to_string(i+1));
            if(model_type_to_make!=1) new_file_name.append("-1");
            new_file_name.append("r.pdb");
            ofstream new_file(new_file_name.c_str());
            if(new_file==0)
            {
                cout<<"ERROR: Could not create a new file\n";
                log_file<<"ERROR: Could not create a new file"<<endl;
                log_file.close();
                return 1;
            }
            string line;
            while(getline(org_file,line))
            {
                new_file<<line<<endl;
            }
            file_test.close();
            new_file.close();
            break;
        }
        file_test.close();
    }

    //damsel
    cout<<"Damsel analysis...\n";
    log_file<<"Damsel analysis..."<<endl;
    command_line=string("damsel -s ");
    command_line.append(symmetry_input);
    command_line.append(" ");
    for(int i=0;i<number_of_models_to_make;i++)
    {
        command_line.append(float_to_string(i+1));
        if(model_type_to_make!=1) command_line.append("-1");
        command_line.append("r.pdb ");
    }
    system(command_line.c_str());
    cout<<"\nDamsel analysis complete\n\n";
    log_file<<"...done"<<endl;;

    //average
    cout<<"Averaging models...\n";
    log_file<<"Averaging models..."<<endl;
    command_line=string("damaver -s ");
    command_line.append(symmetry_input);
    command_line.append(" ");
    for(int i=0;i<number_of_models_to_make;i++)
    {
        command_line.append(float_to_string(i+1));
        if(model_type_to_make!=1) command_line.append("-1");
        command_line.append("r.pdb ");
    }
    system(command_line.c_str());
    cout<<"\nAveraging complete\n\n";
    log_file<<"...done"<<endl;;

    //filter
    cout<<"Filtering averaged model...\n";
    log_file<<"Filtering averaged model..."<<endl;
    command_line=string("damfilt damaver.pdb");
    system(command_line.c_str());
    cout<<"\nModel filtering complete\n\n";
    log_file<<"...done"<<endl;

    //damstart
    cout<<"Running damstart...\n";
    log_file<<"Running damstart..."<<endl;
    command_line=string("damstart damaver.pdb");
    system(command_line.c_str());
    cout<<"\ndamstart complete\n\n";
    log_file<<"...done"<<endl;

    //make summary table
    ofstream res_sum_file("results.txt");
    if(res_sum_file==0)
    {
        cout<<"ERROR: Could not create results file\n";
    }
    else
    {
        //write model values in a table
        float chi2_aver=0;
        float vol_aver=0;
        res_sum_file<<"Model\tVolume (Å^3)\tchi^2"<<endl;
        for(int i=0;i<number_of_models_to_make;i++)
        {
            //model name
            res_sum_file<<i+1<<"\t";
            stringstream ss;
            ss<<(i+1);

            //get vol
            switch(model_type_to_make)
            {
                case 1://gasbor
                {
                    string filename_pdb(ss.str());
                    filename_pdb.append("r.pdb");
                    ifstream pdb_file(filename_pdb.c_str());
                    if(pdb_file==0)
                    {
                        cout<<"ERROR: Could not find .pdb file\n";
                    }
                    else
                    {
                        string line;
                        while(true)
                        {
                            if(!getline(pdb_file,line)) break;
                            if(line[20]=='v'&&line[21]=='o'&&line[22]=='l'&&line[23]=='u'&&line[24]=='m'&&line[25]=='e')
                            {
                                string value;
                                bool record_on=false;
                                for(int letter=0;letter<line.length();letter++)
                                {
                                    if(record_on && line[letter]!=' ') value.append(1,line[letter]);
                                    if(!record_on && line[letter]==':') record_on=true;
                                }

                                res_sum_file<<atof(value.c_str())<<"\t\t";
                                vol_aver+=atof(value.c_str());
                                break;
                            }
                        }
                    }
                    pdb_file.close();
                }break;

                case 2://dammin
                {
                    string filename_pdb(ss.str());
                    filename_pdb.append("-1r.pdb");
                    ifstream pdb_file(filename_pdb.c_str());
                    if(pdb_file==0)
                    {
                        cout<<"ERROR: Could not find .pdb file\n";
                    }
                    else
                    {
                        while(true)
                        {
                            string line;
                            if(!getline(pdb_file,line)) break;
                            if(line[20]=='v'&&line[21]=='o'&&line[22]=='l'&&line[23]=='u'&&line[24]=='m'&&line[25]=='e')
                            {
                                string value;
                                bool record_on=false;
                                for(int letter=0;letter<line.length();letter++)
                                {
                                    if(record_on && line[letter]!=' ') value.append(1,line[letter]);
                                    if(!record_on && line[letter]==':') record_on=true;
                                }

                                res_sum_file<<atof(value.c_str())<<"\t\t";
                                vol_aver+=atof(value.c_str());
                                break;
                            }
                        }
                    }
                    pdb_file.close();
                }break;

                case 3://dammif
                {
                    string filename_pdb(ss.str());
                    filename_pdb.append("-1r.pdb");
                    ifstream pdb_file(filename_pdb.c_str());
                    if(pdb_file==0)
                    {
                        cout<<"ERROR: Could not find .pdb file\n";
                    }
                    else
                    {
                        while(true)
                        {
                            string line;
                            if(!getline(pdb_file,line)) break;
                            if(line[31]=='v'&&line[32]=='o'&&line[33]=='l'&&line[34]=='u'&&line[35]=='m'&&line[36]=='e')
                            {
                                string value;
                                bool record_on=false;
                                for(int letter=0;letter<line.length();letter++)
                                {
                                    if(record_on && line[letter]!=' ') value.append(1,line[letter]);
                                    if(!record_on && line[letter]==':') record_on=true;
                                }

                                res_sum_file<<atof(value.c_str())<<"\t\t";
                                vol_aver+=atof(value.c_str());
                                break;
                            }
                        }
                    }
                    pdb_file.close();
                }break;
            }

            //get fit
            string filename(ss.str());
            filename.append(".fir");
            ifstream fit_file(filename.c_str());
            if(fit_file==0)
            {
                cout<<"ERROR: Could not find .fir file\n";
            }
            else
            {
                string line;
                getline(fit_file,line);
                string value;
                bool record_on=false;
                for(int letter=0;letter<line.length();letter++)
                {
                    if(record_on && line[letter]!=' ') value.append(1,line[letter]);
                    if(!record_on && line[letter]=='=') record_on=true;
                }

                res_sum_file<<atof(value.c_str())<<endl;
                chi2_aver+=atof(value.c_str());
            }
            fit_file.close();

        }

        //average values
        vol_aver/=(float)number_of_models_to_make;
        chi2_aver/=(float)number_of_models_to_make;
        res_sum_file<<"Average\t"<<vol_aver<<"\t"<<chi2_aver<<endl<<endl;

        //insert damaver values (damaver values in damfilt)
        /*ifstream damaver_file("damaver.pdb");
        if(damaver_file==0)
        {
            cout<<"ERROR: Could not find damaver.pdb file\n";
        }
        else
        {
            while(true)
            {
                string line;
                getline(damaver_file,line);
                if(line[29]=='v'&&line[30]=='o'&&line[31]=='l'&&line[32]=='u'&&line[33]=='m'&&line[34]=='e')
                {
                    string value;
                    bool record_on=false;
                    for(int letter=0;letter<line.length();letter++)
                    {
                        if(record_on && line[letter]!=' ') value.append(1,line[letter]);
                        if(!record_on && line[letter]==':') record_on=true;
                    }

                    res_sum_file<<"damaver "<<atof(value.c_str())<<endl;
                    break;
                }
            }
        }
        damaver_file.close();*/

        //insert damfilt values
        ifstream damfilt_file("damfilt.pdb");
        if(damfilt_file==0)
        {
            cout<<"ERROR: Could not find damfilt.pdb file\n";
        }
        else
        {
            int numof_atoms_before=0;
            int numof_atoms_after=0;
            float vol_per_atom=0;

            while(true)
            {
                string line;
                getline(damfilt_file,line);
                /*if(line[29]=='v'&&line[30]=='o'&&line[31]=='l'&&line[32]=='u'&&line[33]=='m'&&line[34]=='e')
                {
                    string value;
                    bool record_on=false;
                    for(int letter=0;letter<line.length();letter++)
                    {
                        if(record_on && line[letter]!=' ') value.append(1,line[letter]);
                        if(!record_on && line[letter]==':') record_on=true;
                    }

                }*/

                if(line[22]=='a'&&line[23]=='t'&&line[24]=='o'&&line[25]=='m'&&line[26]=='s')
                {
                    string value;
                    bool record_on=false;
                    for(int letter=0;letter<line.length();letter++)
                    {
                        if(record_on && line[letter]!=' ') value.append(1,line[letter]);
                        if(!record_on && line[letter]==':') record_on=true;
                    }
                    numof_atoms_before=atoi(value.c_str());
                }
                if(line[28]=='p'&&line[29]=='e'&&line[30]=='r'&&line[31]==' '&&line[32]=='a')
                {
                    string value;
                    bool record_on=false;
                    for(int letter=0;letter<line.length();letter++)
                    {
                        if(record_on && line[letter]!=' ') value.append(1,line[letter]);
                        if(!record_on && line[letter]==':') record_on=true;
                    }
                    vol_per_atom=atof(value.c_str());
                }
                if(line[31]=='a'&&line[32]=='t'&&line[33]=='o'&&line[34]=='m'&&line[35]=='s')
                {
                    string value;
                    bool record_on=false;
                    for(int letter=0;letter<line.length();letter++)
                    {
                        if(record_on && line[letter]!=' ') value.append(1,line[letter]);
                        if(!record_on && line[letter]==':') record_on=true;
                    }
                    numof_atoms_after=atoi(value.c_str());

                    //all values found
                    break;
                }
            }

            res_sum_file<<"damaver "<<numof_atoms_before*vol_per_atom<<endl;
            res_sum_file<<"damfilt "<<numof_atoms_after*vol_per_atom<<endl;
        }
        damfilt_file.close();
    }
    res_sum_file.close();

    //complete
    timer=time(0);
    now=localtime(&timer);
    time_stamp=string("");
    if(now->tm_hour<10) time_stamp.append("0");
    time_stamp.append(float_to_string(now->tm_hour));
    time_stamp.append(":");
    if(now->tm_min<10) time_stamp.append("0");
    time_stamp.append(float_to_string(now->tm_min));
    time_stamp.append(":");
    if(now->tm_sec<10) time_stamp.append("0");
    time_stamp.append(float_to_string(now->tm_sec));
    cout<<"\nProcedure completed at "<<time_stamp<<endl<<endl;
    log_file<<"\nProcedure completed at "<<time_stamp<<endl;
    log_file.close();

    cout<<"Press ENTER to exit\n";
    getline(cin,user_input_string);

    return 0;
}

float string_to_float(string word)
{
    float value=0;
    istringstream(word)>>value;
    return value;
}

int string_to_int(string word)
{
    int value=0;
    istringstream(word)>>value;
    return value;
}

string float_to_string(float value)
{
    stringstream ss;
    ss<<value;
    return string(ss.str());
}
