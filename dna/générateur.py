import random

# Définition des caractères possibles
nucleotides = ['A', 'T', 'C', 'G']

# Fonction pour générer une séquence aléatoire de longueur donnée
def generate_sequence(length):
    return ''.join(random.choices(nucleotides, k=length))

# Nombre de séquences à générer
num_sequences = 100000

# Longueur de chaque séquence
sequence_length = 50

# Nom du fichier de sortie
output_file = 'sequences.fasta'

# Génération des séquences et écriture dans le fichier FASTA
with open(output_file, 'w') as f:
    for i in range(1, num_sequences + 1):
        sequence = generate_sequence(sequence_length)
        f.write(f'{sequence}\n')

print(f'Fichier {output_file} généré avec succès.')
