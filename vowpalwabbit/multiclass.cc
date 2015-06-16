#include <cstring>
#include <climits>
#include "global_data.h"
#include "vw.h"

namespace MULTICLASS {

  char* bufread_label(label_t* ld, char* c)
  {
    memcpy(&ld->label, c, sizeof(ld->label));
    c += sizeof(ld->label);
    memcpy(&ld->weight, c, sizeof(ld->weight));
    c += sizeof(ld->weight);
    return c;
  }
  
  size_t read_cached_label(shared_data*, void* v, io_buf& cache)
  {
    label_t* ld = (label_t*) v;
    char *c;
    size_t total = sizeof(ld->label)+sizeof(ld->weight);
    if (buf_read(cache, c, total) < total) 
      return 0;
    c = bufread_label(ld,c);
    
    return total;
  }
  
  float weight(void* v)
  {
    label_t* ld = (label_t*) v;
    return (ld->weight > 0) ? ld->weight : 0.f;
  }
  
  char* bufcache_label(label_t* ld, char* c)
  {
    memcpy(c, &ld->label, sizeof(ld->label));
    c += sizeof(ld->label);
    memcpy(c, &ld->weight, sizeof(ld->weight));
    c += sizeof(ld->weight);
    return c;
  }

  void cache_label(void* v, io_buf& cache)
  {
    char *c;
    label_t* ld = (label_t*) v;
    buf_write(cache, c, sizeof(ld->label)+sizeof(ld->weight));
    c = bufcache_label(ld,c);
  }

  void default_label(void* v)
  {
    label_t* ld = (label_t*) v;
    ld->label = (uint32_t)-1;
    ld->weight = 1.;
  }

  void delete_label(void*) {}

  void parse_label(parser*, shared_data*sd, void* v, v_array<substring>& words)
  {
    label_t* ld = (label_t*)v;

    switch(words.size()) {
    case 0:
      break;
    case 1:
      ld->label = sd->ldict ? sd->ldict->get(words[0]) : int_of_substring(words[0]);
      ld->weight = 1.0;
      break;
    case 2:
      ld->label = sd->ldict ? sd->ldict->get(words[0]) : int_of_substring(words[0]);
      ld->weight = float_of_substring(words[1]);
      break;
    default:
      cerr << "malformed example!\n";
      cerr << "words.size() = " << words.size() << endl;
    }
    if (ld->label == 0)
      {
		  stringstream msg;
		  msg << "label 0 is not allowed for multiclass.  Valid labels are {1,k}";
		  if (sd->ldict) msg << endl << "this likely happened because you specified an invalid label with named labels";
		  cout << msg.str() << endl;
		  throw runtime_error(msg.str().c_str());
      }
  }

  label_parser mc_label = {default_label, parse_label, 
				  cache_label, read_cached_label, 
				  delete_label, weight, 
				  nullptr,
				  sizeof(label_t)};
  
  void print_update(vw& all, example &ec)
  {
    if (all.sd->weighted_examples >= all.sd->dump_interval && !all.quiet && !all.bfgs)
      {
        if (! all.sd->ldict)
	all.sd->print_update(all.holdout_set_off, all.current_pass, ec.l.multi.label, ec.pred.multiclass,
			     ec.num_features, all.progress_add, all.progress_arg);
        else {
          substring ss_label = all.sd->ldict->get(ec.l.multi.label);
          substring ss_pred  = all.sd->ldict->get(ec.pred.multiclass);
          all.sd->print_update(all.holdout_set_off, all.current_pass,
                               !ss_label.begin ? "unknown" : string(ss_label.begin, ss_label.end - ss_label.begin),
                               !ss_pred.begin  ? "unknown" : string(ss_pred.begin, ss_pred.end - ss_pred.begin),
                               ec.num_features, all.progress_add, all.progress_arg);
      }
  }
  }

  void finish_example(vw& all, example& ec)
  {
    float loss = 1;
    if (ec.l.multi.label == (uint32_t)ec.pred.multiclass)
      loss = 0;
    
    all.sd->update(ec.test_only, loss, ec.l.multi.weight, ec.num_features);
    
    for (int* sink = all.final_prediction_sink.begin; sink != all.final_prediction_sink.end; sink++)
      if (! all.sd->ldict)
      all.print(*sink, (float)ec.pred.multiclass, 0, ec.tag);
      else {
        substring ss_pred = all.sd->ldict->get(ec.pred.multiclass);
        all.print_text(*sink, string(ss_pred.begin, ss_pred.end - ss_pred.begin), ec.tag);
      }
    
    MULTICLASS::print_update(all, ec);
    VW::finish_example(all, &ec);
  }
}
