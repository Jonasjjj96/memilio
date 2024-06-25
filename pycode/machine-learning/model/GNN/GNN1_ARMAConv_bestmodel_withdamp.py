import os
import pickle
import spektral
import json
import pandas as pd
import numpy as np
from sklearn.preprocessing import FunctionTransformer

# from first_attempt_withSpectral_withCV import preprocess, train_step, evaluate, test_evaluation
import matplotlib.pyplot as plt
import time

import numpy as np
import scipy.sparse as sp
import tensorflow as tf
from keras.layers import Dense
from keras.losses import MeanAbsolutePercentageError
from keras.metrics import mean_absolute_percentage_error
from keras.models import Model
#from keras.optimizers.legacy import Adam, Nadam, RMSprop, SGD, Adagrad
from keras.optimizers import Adam, Nadam, RMSprop, SGD, Adagrad

from sklearn.model_selection import KFold

from spektral.data import Dataset, DisjointLoader, Graph, Loader, BatchLoader, MixedLoader
from spektral.layers import GCSConv, GlobalAvgPool, GlobalAttentionPool, ARMAConv, AGNNConv, APPNPConv, CrystalConv, GATConv, GINConv, XENetConv, GCNConv
from spektral.transforms.normalize_adj import NormalizeAdj
from spektral.utils.convolution import gcn_filter, normalized_laplacian, rescale_laplacian, normalized_adjacency

# load and prepare data
#path = os.path.dirname(os.path.realpath(__file__))#

#path_data = os.path.join(
#    os.path.dirname(
#        os.path.realpath(os.path.dirname(os.path.realpath(path)))),
#    'data_GNN_400pop_4var_damp_100days_1k_w')


#file = open(os.path.join(path_data, 'GNN_400pop_damp_w.pickle'), 'rb')
file = open('/localdata1/schm_a45/dampings/data_GNN_100days_400pop_5damp_2024_new/GNN_400pop_damp_w.pickle', 'rb')

data_secir = pickle.load(file)




#len_dataset = data_secir['inputs'][0].shape[0]
#numer_of_nodes = np.asarray(data_secir['inputs']).shape[0]
#shape_input_flat = np.asarray(
#    data_secir['inputs']).shape[2]*np.asarray(data_secir['inputs']).shape[3]
#shape_labels_flat = np.asarray(
#    data_secir['labels']).shape[2]*np.asarray(data_secir['labels']).shape[3]


#new_inputs = np.asarray(
#    data_secir['inputs']).reshape(
#    len_dataset, numer_of_nodes, shape_input_flat)
#new_labels = np.asarray(data_secir['labels']).reshape(
#    len_dataset, numer_of_nodes, shape_labels_flat)

len_dataset = 1000

numer_of_nodes = 400
shape_input_flat = np.asarray(
    data_secir['inputs'][1]).shape[1]*np.asarray(data_secir['inputs'][1]).shape[2]
shape_labels_flat = np.asarray(
    data_secir['labels'][1]).shape[1]*np.asarray(data_secir['labels'][1]).shape[2]


### forgot to scale data in data generation 
transformer = FunctionTransformer(np.log1p, validate=True)
# inputs = np.asarray(
#     data['inputs']).transpose(
            #     2, 0, 1)
inputs = np.asarray(
data_secir['inputs'][1]).transpose(2,0,1,3).reshape(48,-1)
scaled_inputs = transformer.transform(inputs)
scaled_inputs = scaled_inputs.transpose().reshape(len_dataset, 400,5, 48)

labels = np.asarray(
            data_secir['labels'][1]).transpose(2,0,1,3).reshape(48,-1)
scaled_labels = transformer.transform(labels)
scaled_labels = scaled_labels.transpose().reshape(len_dataset, 400,100, 48)

new_inputs = np.asarray(
    scaled_inputs).reshape(
    len_dataset, numer_of_nodes, shape_input_flat)
new_labels = np.asarray(scaled_labels).reshape(
    len_dataset, numer_of_nodes, 100*48)





n_days = int(new_labels.shape[2]/48)

damping_factors = data_secir['damping_coeff'][0] # one for every run
damping_days = data_secir['damping_day'][0]# one for every rum 
contact_matrices = data_secir['damped_matrix'][0]

n_runs = new_inputs.shape[0]
n_pop = new_inputs.shape[1]
n_dampings = np.asarray(data_secir['damping_day']).shape[2]
#n_dampings = 1

# inputs_with_damp = np.dstack((new_inputs,(np.asarray(damping_factors).reshape([n_runs,n_pop,n_dampings])),
#                                (np.asarray(damping_days).reshape([n_runs,n_pop,n_dampings])),
#                                (np.asarray(contact_matrices).reshape([n_runs,n_pop,36*n_dampings]))))

inputs_with_damp = np.dstack((new_inputs,np.repeat(np.asarray(damping_factors)[:, np.newaxis, :], 400, axis=1),
                               (np.repeat(np.asarray(damping_days)[:, np.newaxis, :], 400, axis=1)),
                               (np.repeat(np.asarray(contact_matrices)[:, np.newaxis, :], 400, axis=1).reshape([n_runs,n_pop,36*n_dampings]))))

### for one damp: 
#damping_factors_reshaped =  np.tile(np.asarray(damping_factors)[:, np.newaxis], (1, 400))
#damping_days_reshaped = np.tile(np.asarray(damping_days)[:, np.newaxis], (1, 400))
#contact_matrices_reshaped = np.repeat(np.asarray(contact_matrices)[:, np.newaxis, :, :], 400, axis=1).reshape([n_runs,n_pop,-1])
#inputs_with_damp = np.concatenate((new_inputs,
#                                   np.expand_dims(damping_factors_reshaped,axis = 2),
#                                   np.expand_dims(damping_days_reshaped, axis = 2),
#                                   contact_matrices_reshaped), axis = 2  )


######## open commuter data #########
path = os.path.dirname(os.path.realpath(__file__))
path_data = os.path.join(path,
                         'data')
commuter_file = open(os.path.join(
    path_data, 'commuter_migration_scaled.txt'), 'rb')
commuter_data = pd.read_csv(commuter_file, sep=" ", header=None)
sub_matrix = commuter_data.iloc[:numer_of_nodes, 0:numer_of_nodes]


adjacency_matrix = np.asarray(sub_matrix)

adjacency_matrix[adjacency_matrix > 0] = 1
node_features = inputs_with_damp

node_labels = new_labels

layer = 'ARMAConv'
number_of_layers = 4
number_of_channels = 512

parameters = [layer, number_of_layers, number_of_channels]


df = pd.DataFrame(
    columns=['layer', 'number_of_layers', 'channels', 'kfold_train',
             'kfold_val', 'kfold_test', 'training_time',
             'train_losses', 'val_losses'])


def train_and_evaluate_model(
        epochs, learning_rate, param, save_name, filename):

    layer_name, number_of_layer, channels = param

    if layer_name =='GCNConv':
        layer = GCNConv
    elif layer_name=='ARMAConv':
        layer = ARMAConv
    elif layer_name == 'GATConv':
        layer = GATConv
    elif layer_name == 'APPNPConv':
        layer = APPNPConv
    elif layer_name == 'GCSConv':
        layer = GCNConv

    class MyDataset(spektral.data.dataset.Dataset):
        def read(self):
            if layer_name == 'GCNConv':
                self.a = gcn_filter(adjacency_matrix)
            elif layer_name == 'ARMAConv':
                self.a = rescale_laplacian(
                    normalized_laplacian(adjacency_matrix))
            elif layer_name == 'APPNPConv':
                self.a = gcn_filter(adjacency_matrix)
            elif layer_name == 'GATConv':
                self.a = adjacency_matrix
            elif layer_name == 'GCSConv':
                self.a = normalized_adjacency(adjacency_matrix)


            # self.a = normalized_adjacency(adjacency_matrix)
            # self.a = rescale_laplacian(normalized_laplacian(adjacency_matrix))

            return [spektral.data.Graph(x=x, y=y) for x, y in zip(node_features, node_labels)]

            super().__init__(**kwargs)

    # data = MyDataset()
    data = MyDataset(transforms=NormalizeAdj())
    batch_size = 32
    epochs = epochs
    es_patience = 100  # Patience for early stopping

    if number_of_layer == 1:
        class Net(Model):
            def __init__(self):
                super().__init__()
                self.conv1 = layer(channels, activation='elu')
                self.dense = Dense(data.n_labels, activation="linear")

            def call(self, inputs):
                x, a = inputs
                a = np.asarray(a)

                x = self.conv1([x, a])

                output = self.dense(x)

                return output

    elif number_of_layer == 2:
        class Net(Model):
            def __init__(self):
                super().__init__()
                self.conv1 = layer(channels,  activation='elu')
                self.conv2 = layer(channels,  activation='elu')
                self.dense = Dense(data.n_labels, activation="linear")

            def call(self, inputs):
                x, a = inputs
                a = np.asarray(a)

                x = self.conv1([x, a])
                x = self.conv2([x, a])

                output = self.dense(x)

                return output

    elif number_of_layer == 4:
        class Net(Model):
            def __init__(self):
                super().__init__()

                self.conv1 = layer(channels, order = 3, activation='relu')
                self.conv2 = layer(channels,  order = 3,activation='relu')
                self.conv3 = layer(channels, order = 3,activation='relu')
                self.conv4 = layer(channels, order = 3,activation='relu')
                self.dense = Dense(data.n_labels, activation="linear")

            def call(self, inputs):
                x, a = inputs
                #a = np.asarray(a)

                x = self.conv1([x, a])
                x = self.conv2([x, a])
                x = self.conv3([x, a])
                x = self.conv4([x,a])

                output = self.dense(x)

                return output

    optimizer = Adam(learning_rate=learning_rate)
    #optimizer = optimizer(learning_rate=learning_rate)
    loss_fn = MeanAbsolutePercentageError()

    def train_step(inputs, target):
        with tf.GradientTape() as tape:
            predictions = model(inputs, training=True)
            loss = loss_fn(target, predictions) + sum(model.losses)
        gradients = tape.gradient(loss, model.trainable_variables)
        optimizer.apply_gradients(zip(gradients, model.trainable_variables))
        acc = tf.reduce_mean(
            mean_absolute_percentage_error(
                target, predictions))
        return loss, acc

    def evaluate(loader):
        output = []
        step = 0
        while step < loader.steps_per_epoch:
            step += 1
            inputs, target = loader.__next__()
            pred = model(inputs, training=False)
            outs = (
                loss_fn(target, pred),
                tf.reduce_mean(mean_absolute_percentage_error(target, pred)),
                len(target),  # Keep track of batch size
            )
            output.append(outs)
            if step == loader.steps_per_epoch:
                output = np.array(output)
                return np.average(output[:, :-1], 0, weights=output[:, -1])

    n_days = int(new_labels.shape[2]/48)

    def test_evaluation(loader):

        inputs, target = loader.__next__()
        pred = model(inputs, training=False)

        mean_per_batch = []
        states_array = []
        # for i in InfectionState.values():
        #     states_array.append(i)

        for batch_p, batch_t in zip(pred, target):
            MAPE_v = []
            for v_p, v_t in zip(batch_p, batch_t):

                pred_ = tf.reshape(v_p, (n_days, 48))
                target_ = tf.reshape(v_t, (n_days, 48))

                diff = pred_ - target_
                relative_err = (abs(diff))/abs(target_)
                relative_err_transformed = np.asarray(
                    relative_err).transpose().reshape(8, -1)
                relative_err_means_percentage = relative_err_transformed.mean(
                    axis=1) * 100

                MAPE_v.append(relative_err_means_percentage)

            mean_per_batch.append(np.asarray(MAPE_v).transpose().mean(axis=1))
        infectionstates = ['Susceptible','Exposed', 'InfectedNoSymptoms', 'InfectedSymptoms', 'InfectedSevere', 'InfectedCritical', 'Receovered', 'Dead']
        mean_percentage = pd.DataFrame(
            data=np.asarray(mean_per_batch).transpose().mean(axis=1),
            # index=[str(compartment).split('.')[1]
            #        for compartment in states_array[:8]],
            index=[str(compartment).split('.')[1]
                   for compartment in infectionstates],
            
            # index=[str(compartment).split('.')[1]
            #        for compartment in InfectionState.values()],
            columns=['Percentage Error'])

        return mean_percentage



    test_scores = []
    train_losses = []
    val_losses = []

    losses_history_all = []
    val_losses_history_all = []

    start = time.perf_counter()

    model = Net()
    #idxs = np.random.permutation(len(data)) #random 
    idxs = np.arange(len(data))#same for each run 
    split_va, split_te = int(0.8 * len(data)), int(0.9 * len(data))
    idx_tr, idx_va, idx_te = np.split(idxs, [split_va, split_te])

    data_tr = data[idx_tr]
    data_va = data[idx_va]
    data_te = data[idx_te]


    # Data loaders
    loader_tr = MixedLoader(
            data_tr, batch_size=batch_size, epochs=epochs)
    loader_va = MixedLoader(data_va,  batch_size=batch_size)
    loader_te = MixedLoader(data_te, batch_size=data_te.n_graphs)

    epoch = step = 0
    best_val_loss = np.inf
    best_weights = None
    patience = es_patience
    results = []
    losses_history = []
    val_losses_history = []

    start = time.perf_counter()

    for batch in loader_tr:
            step += 1
            loss, acc = train_step(*batch)
            results.append((loss, acc))
            if step == loader_tr.steps_per_epoch:
                step = 0
                epoch += 1

                # Compute validation loss and accuracy
                val_loss, val_acc = evaluate(loader_va)
                print(
                    "Ep. {} - Loss: {:.3f} - Acc: {:.3f} - Val loss: {:.3f} - Val acc: {:.3f}".format(
                        epoch, *np.mean(results, 0), val_loss, val_acc
                    )
                )

                # Check if loss improved for early stopping
                if val_loss < best_val_loss:
                    best_val_loss = val_loss
                    patience = es_patience
                    print("New best val_loss {:.3f}".format(val_loss))
                    best_weights = model.get_weights()
                else:
                    patience -= 1
                    if patience == 0:
                        print(
                            "Early stopping (best val_loss: {})".format(
                                best_val_loss))
                        break
                results = []
                losses_history.append(loss)
                val_losses_history.append(val_loss)

    ################################################################################
    # Evaluate model
    ################################################################################
    model.set_weights(best_weights)  # Load best model
    test_loss, test_acc = evaluate(loader_te)
    #test_MAPE = test_evaluation(loader_te)
    # print(test_MAPE)

    print(
            "Done. Test loss: {:.4f}. Test acc: {:.2f}".format(
                test_loss, test_acc))
    test_scores.append(test_loss)
    train_losses.append(np.asarray(losses_history).min())
    val_losses.append(np.asarray(val_losses_history).min())
    losses_history_all.append(np.asarray(losses_history))
    val_losses_history_all.append(val_losses_history)

    elapsed = time.perf_counter() - start

    # save best weights as pickle 
    with open("best_weights_ARMAConv_100days_5damp_graphdata_new.pickle", 'wb') as f:
             pickle.dump(best_weights, f) 


    # plot the losses
    # plt.figure()
    # plt.plot(np.asarray(losses_history_all).mean(axis=0), label='train loss')
    # plt.plot(np.asarray(val_losses_history_all).mean(axis=0), label='val loss')
    # plt.xlabel('Epoch')
    # plt.ylabel('Loss ( MAPE)')
    # plt.title('Loss for' + str(layer))
    # plt.legend()
    # plt.savefig('losses'+str(layer)+'.png')

    # print out stats
    print("Best train losses: {} ".format(train_losses))
    print("Best validation losses: {}".format(val_losses))
    print("Test values: {}".format(test_scores))
    print("--------------------------------------------")
    print("K-Fold Train Score:{}".format(np.mean(train_losses)))
    print("K-Fold Validation Score:{}".format(np.mean(val_losses)))
    print("K-Fold Test Score: {}".format(np.mean(test_scores)))

    print("Time for training: {:.4f} seconds".format(elapsed))
    print("Time for training: {:.4f} minutes".format(elapsed/60))

    #save the model
    # path = os.path.dirname(os.path.realpath(__file__))
    # path_models = os.path.join(
    #     os.path.dirname(
    #         os.path.realpath(os.path.dirname(os.path.realpath(path)))),
    #     save_name)
    # if not os.path.isdir(path_models):
    #     os.mkdir(path_models)

    # model_json = model.to_json()
    # # Store the JSON data in a file
    # with open("GNN_90days.json", "w") as file:
    #     json.dump(model_json, file)

    # #model.save(path_models, save_name +'h5')
    # model.save_weights(path_models+'.h5')


    # save df 


    df.loc[len(df.index)] = [layer_name, number_of_layer, channels, 
                             np.mean(train_losses),
                             np.mean(val_losses),
                             np.mean(test_scores),
                             (elapsed / 60),
                             [losses_history_all],
                             [val_losses_history_all]]
    #print(df)
    # [np.asarray(losses_history_all).mean(axis=0)],
    # [np.asarray(val_losses_history_all).mean(axis=0)]]

    path = os.path.dirname(os.path.realpath(__file__))
    file_path = os.path.join(
       os.path.dirname(
           os.path.realpath(os.path.dirname(os.path.realpath(path)))),
       'model_evaluation_graphdata')
    if not os.path.isdir(file_path):
       os.mkdir(file_path)
    file_path = file_path+filename
    df.to_csv(file_path)


start_hyper = time.perf_counter()
epochs = 1500
filename = '/GNN_ARMA_100days_5damp_graphdata_new'
save_name = 'egal'
#for param in parameters:
train_and_evaluate_model(epochs, 0.001, parameters, save_name, filename)

elapsed_hyper = time.perf_counter() - start_hyper
print(
    "Time for hyperparameter testing: {:.4f} minutes".format(
        elapsed_hyper / 60))
print(
    "Time for hyperparameter testing: {:.4f} hours".format(
        elapsed_hyper / 60 / 60))
