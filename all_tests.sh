#!/bin/bash


# Function to run the test script
run_test_script() {
    echo "*****************************************************************************************"
    echo "Running tests in $1..."
    rm -rf ./database
    mkdir ./database
    rm -f ./current_username.txt
    python3 "$1"
    echo "Tests in $1 finished."
    echo " "
    echo " "
}

# Main function
main() {

    tests_folder="./tests"
    for file in $(ls -1v "$tests_folder"/*.py); do
        run_test_script "$file"
    done
}

# Call the main function
main
