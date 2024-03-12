# 1 Cas traités

## 1.1 Cas 1
### 1.1.1 Description
Séquence de taille élevée par rapport au nombre et à la longueur des patterns
### 1.1.2 Approche
Découpage de la séquence en sous-séquences de taille égale, puis comptage du nombre de correspondance pour chaque sous-séquence pour l'ensemble.
### 1.1.3 Raisons
Si la séquence est de taille élevée, il est plus logique de paralléliser sur la séquence plutot que sur les patterns.



## 1.2 Cas 2
### 1.2.1 Description
Patterns de tailles similaires, nombre de patterns élevé par rapport à la taille de la séquence et au nombre de machines.
Ici, similaire veut dire qu'il n'y a pas de pattern $p$ suffisamment long pour que $len(p)^2 > \sum_i len(p_i)^2 /n$, où $n$ est le nombre de machines et $p_i$ est le i-ème pattern.
Peut aussi traiter les cas où les patterns sont de tailles très différentes, mais n'est pas optimisé pour cela.
### 1.2.2 Approche
Repartition des patterns sur les machines via MPI, puis chaque machine traite les patterns qui lui sont attribués en parallèle grace à OpenMP.
### 1.2.3 Raisons
Si le nombre de patterns est élevé par rapport à la taille de la séquence, il est plus logique de paralléliser sur les patterns plutot que sur la séquence.
Tant que le désequilibre des longueurs des patterns ne cause pas une nécessité de répartir un pattern sur plusieurs machines cette approche fonctionne, mais comme le nombre de machines est supposé faible par rapport à la taille de la séquence (voir cas non-traités), cela ne devrait pas arriver.


# 2 Arbre décisionnel :
- si on est dans un cluster mpi :
répartir entre les noeuds
sinon ne rien faire
- répartir les paterns si ça vaut le coup
- Pour chaque noeud :
regarder si on a une carte graphique et que c'est suffisament lourd pour que ça valle le coup (faire une règle sur la taille du texte et la taille des paterns)






