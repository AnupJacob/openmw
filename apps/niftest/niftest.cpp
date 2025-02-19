///Program to test .nif files both on the FileSystem and in BSA archives.

#include <iostream>
#include <filesystem>

#include <components/misc/stringops.hpp>
#include <components/nif/niffile.hpp>
#include <components/files/constrainedfilestream.hpp>
#include <components/vfs/manager.hpp>
#include <components/vfs/bsaarchive.hpp>
#include <components/vfs/filesystemarchive.hpp>

#include <boost/program_options.hpp>

// Create local aliases for brevity
namespace bpo = boost::program_options;

///See if the file has the named extension
bool hasExtension(std::string filename, std::string extensionToFind)
{
    std::string extension = filename.substr(filename.find_last_of('.')+1);
    return Misc::StringUtils::ciEqual(extension, extensionToFind);
}

///See if the file has the "nif" extension.
bool isNIF(const std::string & filename)
{
    return hasExtension(filename,"nif");
}
///See if the file has the "bsa" extension.
bool isBSA(const std::string & filename)
{
    return hasExtension(filename,"bsa");
}

/// Check all the nif files in a given VFS::Archive
/// \note Can not read a bsa file inside of a bsa file.
void readVFS(std::unique_ptr<VFS::Archive>&& anArchive, std::string archivePath = "")
{
    VFS::Manager myManager(true);
    myManager.addArchive(std::move(anArchive));
    myManager.buildIndex();

    for(const auto& name : myManager.getRecursiveDirectoryIterator(""))
    {
        try{
            if(isNIF(name))
            {
            //           std::cout << "Decoding: " << name << std::endl;
                Nif::NIFFile temp_nif(myManager.get(name),archivePath+name);
            }
            else if(isBSA(name))
            {
                if(!archivePath.empty() && !isBSA(archivePath))
                {
//                     std::cout << "Reading BSA File: " << name << std::endl;
                    readVFS(std::make_unique<VFS::BsaArchive>(archivePath + name), archivePath + name + "/");
//                     std::cout << "Done with BSA File: " << name << std::endl;
                }
            }
        }
        catch (std::exception& e)
        {
            std::cerr << "ERROR, an exception has occurred:  " << e.what() << std::endl;
        }
    }
}

bool parseOptions (int argc, char** argv, std::vector<std::string>& files)
{
    bpo::options_description desc("Ensure that OpenMW can use the provided NIF and BSA files\n\n"
        "Usages:\n"
        "  niftool <nif files, BSA files, or directories>\n"
        "      Scan the file or directories for nif errors.\n\n"
        "Allowed options");
    desc.add_options()
        ("help,h", "print help message.")
        ("input-file", bpo::value< std::vector<std::string> >(), "input file")
        ;

    //Default option if none provided
    bpo::positional_options_description p;
    p.add("input-file", -1);

    bpo::variables_map variables;
    try
    {
        bpo::parsed_options valid_opts = bpo::command_line_parser(argc, argv).
            options(desc).positional(p).run();
        bpo::store(valid_opts, variables);
        bpo::notify(variables);
        if (variables.count ("help"))
        {
            std::cout << desc << std::endl;
            return false;
        }
        if (variables.count("input-file"))
        {
            files = variables["input-file"].as< std::vector<std::string> >();
            return true;
        }
    }
    catch(std::exception &e)
    {
        std::cout << "ERROR parsing arguments: " << e.what() << "\n\n"
            << desc << std::endl;
        return false;
    }

    std::cout << "No input files or directories specified!" << std::endl;
    std::cout << desc << std::endl;
    return false;
}

int main(int argc, char **argv)
{
    std::vector<std::string> files;
    if(!parseOptions (argc, argv, files))
        return 1;

    Nif::NIFFile::setLoadUnsupportedFiles(true);
//     std::cout << "Reading Files" << std::endl;
    for(auto it=files.begin(); it!=files.end(); ++it)
    {
        std::string name = *it;

        try
        {
            if(isNIF(name))
            {
                //std::cout << "Decoding: " << name << std::endl;
                Nif::NIFFile temp_nif(Files::openConstrainedFileStream(name), name);
             }
             else if(isBSA(name))
             {
//                 std::cout << "Reading BSA File: " << name << std::endl;
                readVFS(std::make_unique<VFS::BsaArchive>(name));
             }
             else if(std::filesystem::is_directory(std::filesystem::path(name)))
             {
//                 std::cout << "Reading All Files in: " << name << std::endl;
                readVFS(std::make_unique<VFS::FileSystemArchive>(name), name);
             }
             else
             {
                 std::cerr << "ERROR:  \"" << name << "\" is not a nif file, bsa file, or directory!" << std::endl;
             }
        }
        catch (std::exception& e)
        {
            std::cerr << "ERROR, an exception has occurred:  " << e.what() << std::endl;
        }
     }
     return 0;
}
