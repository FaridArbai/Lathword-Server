# Lathword-Server
This is the server for the Latchword messaging application, to be used only in case 
you want to privatize your messaging domain and host your own server instead of 
using my own public and default one.

The deployment of this server by your own would be practical for private environments
where intra-communication is desired but external access is restricted or for
any other situation where exceptional membership is desired. If none of the above
cases fits your own, I encourage you to use the client application with the default
domain that is nested into it. In case you desire to deploy your own Latchword domain,
note that you would have to change and rebuild the original client by changing both
Connection::R_IP and Connection::R_PORT, which currently point to faridarbai.ddns.net::25255.

This server was built from scratch by Farid Arbai, the only contributor for
the Latchword Application. It was developed following a strong procedural and 
data-distributed architecture on it's backend in order to enhace responsiveness
since it was intended to run on a raspberry-pi for monthly electrical cost
reduction. As a consequence, all the data handling is done using the standard 
C++ library instead of a database, therefore enhancing responsiveness and 
latency at the cost of reducing development scalability, which turns out not 
to be a big deal since all the core functionalities are already implemented.

This server has been iteratively tested on all of its functionalities and no
single point of failure has been experienced since its deployment on
03/12/17.

The previous version hosted was removed since there were thousands of lines of
code that were updated through the last months and none of the changes were
commited to github since its original Version Control System was SubVersion.
Due to the deprecation of this VCS, from now and on the server project will be
fully maintained and updated with git so I took the decision of deleting the
uploaded version and reuploading the whole new version to github.

