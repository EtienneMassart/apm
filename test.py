import random
import subprocess
import os

def generate_random_sequences(num_sequences, min_length, max_length):
    return [''.join(random.choices('ACGT', k=random.randint(min_length, max_length))) for _ in range(num_sequences)]

def submit_job_with_dependency(dependency_job_id=None, bash_file_path='test.bash'):
    sbatch_command = ['sbatch']
    if dependency_job_id:
        sbatch_command.extend([f'--dependency=afterany:{dependency_job_id}'])
    sbatch_command.append(bash_file_path)
    result = subprocess.run(sbatch_command, capture_output=True, text=True)
    # Extract job ID from the submission output
    print(result.stdout)
    job_id = result.stdout.strip().split()[-1]
    return job_id

def create_bash_file(random_n, random_apm_arg, sequences, bash_file_path='test.bash'):
    with open(bash_file_path, 'w') as file:
        file.write("#!/bin/bash -l\n\n")
        file.write(f"#SBATCH -n {random_n}\n")
        file.write("#SBATCH -N 1\n\n")
        apm_command = f"mpirun apm {random_apm_arg} dna/small_chrY.fa " + ' '.join(sequences)
        file.write(apm_command + "\n")



last_job_id = None
number_of_repetitions = 10 # Set your desired number of repetitions
job_ids = []

for _ in range(number_of_repetitions):
    random_n = random.randint(1, 8)
    random_apm_arg = random.randint(0, 4)
    num_sequences = random.randint(1, 100)  # Random number of sequences between 1 and 5
    sequences = generate_random_sequences(num_sequences, 4, 15)  # Sequences with length between 5 and 15

    create_bash_file(random_n, random_apm_arg, sequences)
    last_job_id = submit_job_with_dependency(last_job_id)
    job_ids.append(last_job_id)

    os.remove('test.bash')  # Remove the bash file after submitting the job

# Check if segmentation fault occurred
for job_id in job_ids:
    with open(f'slurm-{job_id}.out', 'r') as file:
        if 'error' in file.read():
            print(f"Job {job_id} failed with segmentation fault")

