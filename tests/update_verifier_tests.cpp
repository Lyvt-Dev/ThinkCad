#define NOMINMAX
#include <windows.h>
#include "../packaging/windows/verify_attestation.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
int wmain(int argc,wchar_t** argv){
    if(argc!=4)return 1;const std::wstring verifier=argv[1],installer=argv[2],commit=argv[3];
    if(!thinkercad_update::verify(verifier,installer,commit)){std::cerr<<"Valid provenance rejected\n";return 1;}
    if(thinkercad_update::verify(verifier,installer,std::wstring(40,L'0'))){std::cerr<<"Wrong source commit accepted\n";return 1;}
    const auto folder=std::filesystem::temp_directory_path()/(L"ThinkCad Verify With Spaces "+std::to_wstring(GetCurrentProcessId()));std::filesystem::create_directories(folder);
    const auto copied=folder/std::filesystem::path(installer).filename();std::filesystem::copy_file(installer,copied);std::filesystem::copy_file(installer+L".sigstore.jsonl",copied.wstring()+L".sigstore.jsonl");
    {std::ofstream file(copied,std::ios::binary|std::ios::app);file.put('!');}
    if(thinkercad_update::verify(verifier,copied.wstring(),commit)){std::cerr<<"Tampered installer accepted\n";return 1;}
    std::filesystem::copy_file(installer,copied,std::filesystem::copy_options::overwrite_existing);{std::ofstream file(copied.wstring()+L".sigstore.jsonl",std::ios::binary|std::ios::trunc);file<<"{}";}
    if(thinkercad_update::verify(verifier,copied.wstring(),commit)){std::cerr<<"Invalid attestation accepted\n";return 1;}
    std::filesystem::remove_all(folder);std::cout<<"Installer provenance, commit binding and tamper rejection passed\n";return 0;
}
